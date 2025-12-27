#include "huffman.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int l_type;
    unsigned char l_data;
    int r_type;
    unsigned char r_data;
} ReadNode;

void init_read_node(ReadNode *read_node, int l_type, unsigned char l_data, int r_type, unsigned char r_data) {
    read_node->l_type = l_type;
    read_node->l_data = l_data;
    read_node->r_type = r_type;
    read_node->r_data = r_data;
}

typedef struct HuffmanTree{
    unsigned char symbol;
    int number;
    struct HuffmanTree* left;
    struct HuffmanTree* right;
}HuffmanTree;

void init_huffman_tree(HuffmanTree* tree, HuffmanTree* left, HuffmanTree* right, unsigned char symbol) {
    tree->symbol = symbol;
    tree->left = left;
    tree->right = right;
    tree->number = -1;
}

int is_leaf(const HuffmanTree* tree) {
    return tree->left == NULL && tree->right == NULL;
}

void free_tree(HuffmanTree* tree) {
    if (!tree) return;
    free_tree(tree->left);
    free_tree(tree->right);
    free(tree);
}

//HUFFMAN CODE
void build_frequency_dict(const unsigned char* buffer, size_t buff_size, int freq_dict[256]) {
    for (int i = 0; i < 256; i++) freq_dict[i] = 0;
    for (size_t i = 0; i < buff_size; i++)
        freq_dict[buffer[i]]++;
}

typedef struct QueueNode {
    HuffmanTree* tree;
    int frequency;
    struct QueueNode* next;
}QueueNode;

typedef struct {
    QueueNode* head;
    int size;
}PriorityQueue;

void init_queue(PriorityQueue* queue) {
    queue->head = NULL;
    queue->size = 0;
}

int is_empty(const PriorityQueue* queue) { return queue->head == NULL; }

void enqueue(PriorityQueue* queue, HuffmanTree* tree, const int frequency) {
    QueueNode* new_node = malloc(sizeof(QueueNode));
    new_node->tree = tree;
    new_node->frequency = frequency;
    if (is_empty(queue)) {
        new_node->next = NULL;
        queue->head = new_node;
    } else if (queue->head->frequency > frequency) {
        new_node->next = queue->head;
        queue->head = new_node;
    } else {
        QueueNode* curr = queue->head;
        while (curr->next != NULL && curr->next->frequency <= frequency) curr = curr->next;
        new_node->next = curr->next;
        curr->next = new_node;
    }
    queue->size++;
}

QueueNode* dequeue(PriorityQueue* queue) {
    if (is_empty(queue)) return NULL;
    QueueNode* head = queue->head;
    queue->head = head->next;
    queue->size--;
    return head;
}

HuffmanTree* build_huffman_tree(int freq_dict[256]) {
    PriorityQueue pq;
    init_queue(&pq);
    for (int i = 0; i < 256; i++) {
        if (freq_dict[i] == 0) continue;
        HuffmanTree*tree = malloc(sizeof(HuffmanTree));
        tree->symbol = i;
        tree->left = NULL;
        tree->right = NULL;
        enqueue(&pq, tree, freq_dict[i]);
    }

    if (pq.size == 1) {
        HuffmanTree* dummy = malloc(sizeof(HuffmanTree));
        dummy->symbol = (pq.head->tree->symbol + 1) % 256;
        dummy->left = NULL;
        dummy->right = NULL;

        HuffmanTree* tree = malloc(sizeof(HuffmanTree));
        tree->left = dummy;
        tree->right = pq.head->tree;

        free(pq.head);
        return tree;
    }

    while (pq.size > 1) {
        QueueNode* left_node = dequeue(&pq);
        QueueNode* right_node = dequeue(&pq);
        int new_freq = left_node->frequency + right_node->frequency;
        HuffmanTree* new_tree = malloc(sizeof(HuffmanTree));
        new_tree->left = left_node->tree;
        new_tree->right = right_node->tree;
        enqueue(&pq, new_tree, new_freq);
        free(left_node);
        free(right_node);
    }
    HuffmanTree* root = pq.head->tree;
    free(pq.head);
    return root;
}

typedef struct {
    int code;
    int length;
} HuffmanCode;

void get_codes(const HuffmanTree* tree, const int val, const int depth, HuffmanCode codes[256]) {
    if (tree == NULL) return;

    if (is_leaf(tree)) {
        codes[tree->symbol].code = val;
        codes[tree->symbol].length = depth;
    } else {
        get_codes(tree->left, (val << 1), depth + 1, codes);
        get_codes(tree->right, (val << 1) | 1, depth + 1, codes);
    }
}

int number_nodes(HuffmanTree* tree, int num) {
    if (is_leaf(tree))
        return num;
    num = number_nodes(tree->left, num);
    num = number_nodes(tree->right, num);
    tree->number = num;
    return num + 1;
}

void tree_to_bytes(HuffmanTree* tree, unsigned char *buffer, size_t* index) {
    if (tree && !is_leaf(tree)) {
        tree_to_bytes(tree->left, buffer, index);
        tree_to_bytes(tree->right, buffer, index);
        if (is_leaf(tree->left)) {
            buffer[(*index)++] = 0;
            buffer[(*index)++] = tree->left->symbol;
        } else {
            buffer[(*index)++] = 1;
            buffer[(*index)++] = tree->left->number;
        }
        if (is_leaf(tree->right)) {
            buffer[(*index)++] = 0;
            buffer[(*index)++] = tree->right->symbol;
        } else {
            buffer[(*index)++] = 1;
            buffer[(*index)++] = tree->right->number;
        }
    }
}

typedef struct {
    uint64_t accumulator;
    int count;
    unsigned char* buffer;
    uint64_t buff_index;
} BitBuffer;

void init_bit_buffer(BitBuffer* bit_buffer, unsigned char* buffer, const uint64_t buff_index) {
    bit_buffer->accumulator = 0;
    bit_buffer->count = 0;
    bit_buffer->buffer = buffer;
    bit_buffer->buff_index = buff_index;
}

#define WRITE_BITS(bb, val, nbits) do { \
    (bb)->accumulator |= (uint64_t)(val) << (64 - (nbits) - (bb)->count); \
    (bb)->count += (nbits); \
    while ((bb)->count >= 8) { \
        (bb)->buffer[(bb)->buff_index++] = (bb)->accumulator >> 56; \
        (bb)->accumulator <<= 8; \
        (bb)->count -= 8; \
    } \
} while(0)

#define READ_BITS(bit_buffer, result, nbits) \
    do { \
        int _n = (nbits); \
        while ((bit_buffer)->count < _n) { \
            (bit_buffer)->accumulator = ((bit_buffer)->accumulator << 8) | (bit_buffer)->buffer[(bit_buffer)->buff_index++]; \
            (bit_buffer)->count += 8; \
        } \
        int _shift = (bit_buffer)->count - _n; \
        (result) = ((bit_buffer)->accumulator >> _shift) & ((1ULL << _n) - 1); \
        (bit_buffer)->count = _shift; \
        if ((bit_buffer)->count > 0) \
            (bit_buffer)->accumulator &= (1ULL << (bit_buffer)->count) - 1; \
        else \
            (bit_buffer)->accumulator = 0; \
    } while (0)

#define FLUSH_BITS(bit_buffer) \
    if ((bit_buffer)->count > 0) { \
        (bit_buffer)->buffer[(bit_buffer)->buff_index++] = (unsigned char)((bit_buffer)->accumulator << (8 - (bit_buffer)->count)); \
        (bit_buffer)->count = 0; \
        (bit_buffer)->accumulator = 0; \
    }

unsigned char* compress(const unsigned char *in_buffer, size_t in_size, size_t *out_size) {
    unsigned char* out_buffer = malloc((in_size * 255 + 7) / 8 + 1029);
    if (!out_buffer) return NULL;

    int freq_dict[256];
    build_frequency_dict(in_buffer, in_size, freq_dict);

    HuffmanTree* tree = build_huffman_tree(freq_dict);
    if (!tree) return NULL;

    HuffmanCode codes[256];
    get_codes(tree, 0, 0, codes);

    number_nodes(tree, 0);
    out_buffer[0] = (uint8_t)(tree->number + 1);
    size_t write_index = 1, read_index = 0;
    tree_to_bytes(tree, out_buffer, &write_index);

    const uint32_t length = (uint32_t)in_size;
    out_buffer[write_index++] = (length & 0xFF);
    out_buffer[write_index++] = ((length >> 8) & 0xFF);
    out_buffer[write_index++] = ((length >> 16) & 0xFF);
    out_buffer[write_index++] = ((length >> 24) & 0xFF);

    BitBuffer bit_buffer;
    init_bit_buffer(&bit_buffer, out_buffer, write_index);

    for (size_t i = 0; i < in_size; i++) {
        const HuffmanCode code = codes[in_buffer[i]];
        WRITE_BITS(&bit_buffer, code.code, code.length);
    }
    FLUSH_BITS(&bit_buffer);
    free_tree(tree);
    *out_size = bit_buffer.buff_index;
    return out_buffer;
}

unsigned char *decompress(const unsigned char *in_buffer, size_t in_size, size_t *out_size) { return NULL; }

/*
# ====================
# Functions for decompression

def _generate_tree_general(node_lst: list[ReadNode],
                           root_index: int) -> HuffmanTree:
    """ Return the Huffman tree corresponding to node_lst[root_index].
    The function assumes nothing about the order of the tree nodes in the list.
    """
    root = HuffmanTree()
    node_values = node_lst[root_index]
    if node_values.l_type == 0:
        root.left = HuffmanTree(symbol=node_values.l_data)
    else:
        root.left = _generate_tree_general(node_lst, node_values.l_data)
    if node_values.r_type == 0:
        root.right = HuffmanTree(symbol=node_values.r_data)
    else:
        root.right = _generate_tree_general(node_lst, node_values.r_data)
    return root


def _decompress_bytes(tree: HuffmanTree, text: bytes, size: int) -> bytes:
    """ Use Huffman tree <tree> to decompress <size> bytes from <text>."""
    codes = {value: key for key, value in _get_codes(tree).items()}
    binary_string = "".join(bin(byte)[2:].zfill(8) for byte in text)
    output = bytearray()
    curr = ""
    for char in binary_string:
        curr += char
        if curr in codes:
            output.append(codes[curr])
            curr = ""
    return bytes(output[:size])


def _bytes_to_nodes(buf: bytes) -> list[ReadNode]:
    """ Return a list of ReadNodes corresponding to the bytes in <buf>."""
    return [ReadNode(buf[i], buf[i + 1], buf[i + 2], buf[i + 3]) for i in range(0, len(buf), 4)]


def decompress_file(in_file: str, out_file: str) -> None:
    """ Decompress contents of <in_file> and store results in <out_file>.
    Both <in_file> and <out_file> are string objects representing the names of
    the input and output files.

    Precondition: The contents of the file <in_file> are not empty.
    """
    with open(in_file, "rb") as f:
        num_nodes = f.read(1)[0]
        buf = f.read(num_nodes * 4)
        node_lst = _bytes_to_nodes(buf)
        tree = _generate_tree_general(node_lst, num_nodes - 1)
        size = int.from_bytes(f.read(4), "little")
        with open(out_file, "wb") as g:
            text = f.read()
            g.write(_decompress_bytes(tree, text, size))*/
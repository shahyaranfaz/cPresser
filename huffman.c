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
    if (!new_node) return;
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
        if (!tree) return NULL;
        tree->symbol = i;
        tree->left = NULL;
        tree->right = NULL;
        enqueue(&pq, tree, freq_dict[i]);
    }

    if (pq.size == 1) {
        QueueNode* real = dequeue(&pq);
        HuffmanTree* dummy = malloc(sizeof(HuffmanTree));
        HuffmanTree* root = malloc(sizeof(HuffmanTree));
        if (!dummy || !root) {
            free(dummy); free(root);
            return NULL;
        }
        dummy->symbol = (real->tree->symbol + 1) % 256;
        dummy->left = NULL;
        dummy->right = NULL;
        root->left = dummy;
        root->right = real->tree;

        free(real);
        return root;
    }

    while (pq.size > 1) {
        QueueNode* left_node = dequeue(&pq);
        QueueNode* right_node = dequeue(&pq);
        int new_freq = left_node->frequency + right_node->frequency;
        HuffmanTree* new_tree = malloc(sizeof(HuffmanTree));
        if (!new_tree) return NULL;
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

void tree_to_bytes(const HuffmanTree* tree, unsigned char *buffer, size_t* index) {
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
    size_t write_index = 1;
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

//DECOMPRESS
HuffmanTree* rebuild_huffman_tree(unsigned char* buffer, int index) {
    HuffmanTree* root = malloc(sizeof(HuffmanTree));
    if (!root ) { free_tree(root); return NULL; }
    if (buffer[4 * index] == 0) {
        HuffmanTree* left = malloc(sizeof(HuffmanTree));
        if (!left ) { free_tree(root); free_tree(left); return NULL; }
        left->symbol = buffer[4 * index + 1];
        left->left = NULL;
        left->right = NULL;
        root->left = left;
    } else {
        root->left = rebuild_huffman_tree(buffer, buffer[4 * index + 1]);
    }
    if (buffer[4 * index + 2] == 0) {
        HuffmanTree* right = malloc(sizeof(HuffmanTree));
        if (!right ) { free_tree(root); free_tree(right); return NULL; }
        right->symbol = buffer[4 * index + 3];
        right->left = NULL;
        right->right = NULL;
        root->right = right;
    } else {
        root->right = rebuild_huffman_tree(buffer, buffer[4 * index + 3]);
    }
    return root;
}

unsigned char *decompress(const unsigned char *in_buffer, size_t in_size, size_t *out_size) {
    if (in_size < 5) return NULL;
    size_t write_index = 0, read_index = 0;
    int num_nodes = in_buffer[read_index++];
    HuffmanTree* tree = rebuild_huffman_tree((unsigned char*)in_buffer + 1, num_nodes - 1);
    if (!tree) return NULL;
    read_index += 4 * num_nodes;

    if (read_index + 4 > in_size) {
        free_tree(tree);
        return NULL;
    }

    uint32_t original_size;
    memcpy(&original_size, &in_buffer[read_index], 4);
    read_index += 4;

    unsigned char* out_buffer = malloc(original_size);
    if (!out_buffer) {
        free_tree(tree);
        return NULL;
    }

    BitBuffer bit_buffer;
    init_bit_buffer(&bit_buffer, (unsigned char*)in_buffer, read_index);
    const HuffmanTree* curr = tree;
    while (write_index < original_size) {
        while (!is_leaf(curr)) {
            int bit;
            READ_BITS(&bit_buffer, bit, 1);
            curr = (bit == 0) ? curr->left : curr->right;
        }
        out_buffer[write_index++] = curr->symbol;
        curr = tree;
    }
    free_tree(tree);
    *out_size = write_index;
    return out_buffer;
}
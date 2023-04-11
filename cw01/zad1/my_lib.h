#include <stdlib.h>

struct TextBlock {
    size_t lines;
    char **content;
};


struct MainBlock {
    struct TextBlock *textBlocks;
    size_t mainBlockLimit;
    size_t dataBlockCnt;
};

struct MainBlock *initMainBlock(size_t maxSize);
void addTextBlock(struct MainBlock *mainBlock, char *fileName);
void printTextBlock(struct MainBlock *mainBlock, size_t textBlockIndex);
void deleteTextBlock(struct MainBlock *mainBlock, size_t textBlockIndex);
void deleteMainBlock(struct MainBlock *mainBlock);

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "my_lib.h"

typedef struct MainBlock MainBlock;
typedef struct TextBlock TextBlock;

struct MainBlock *initMainBlock(size_t maxSize) {
    struct MainBlock *block = malloc(sizeof(MainBlock));
    block->textBlocks = calloc(maxSize, sizeof(TextBlock));
    block->mainBlockLimit = maxSize;
    block->dataBlockCnt = 0;
    return block;
}

void addTextBlock(struct MainBlock *mainBlock, char *fileName) {
    if (mainBlock == NULL) {
        printf("MainBlock required.\n");
        return;
    }
    FILE *file = fopen(fileName, "r");
    if (file == NULL) {
        printf("File does not exist.\n");
        return;
    }

    // wc command to file
    char command[100];
    sprintf(command, "mkdir -p tmp && touch tmp/tmp.txt && wc %s > tmp/tmp.txt", fileName);
    system(command);
    FILE *tmp = fopen("tmp/tmp.txt", "r");

    // parse command lines, words
    fgets(command, 100, tmp);
    fclose(tmp);
    //printf("%s", command);
    int lines;
    int words;
    sscanf(command, "%d %d", &lines, &words);

    if (mainBlock->mainBlockLimit >= mainBlock->dataBlockCnt + 1) {

        char **textLines = (char **) calloc(lines, sizeof(char *));
        for (int i = 0; i < lines; i++) {

            // read from file
            size_t lineLen = 0;
            char *tmpLine;
            getline(&tmpLine, &lineLen, file);

            // prepare space and paste
            char *line = calloc(lineLen, sizeof(char));
            strcpy(line, tmpLine);
            free(tmpLine);

            // append line
            textLines[i] = line;
        }

        // create TextBlock
        struct TextBlock *textBlock = malloc(sizeof(TextBlock));
        textBlock->content = textLines;
        textBlock->lines = lines;

        // append TextBlock
        *(mainBlock->textBlocks + mainBlock->dataBlockCnt) = *textBlock;
        mainBlock->dataBlockCnt += 1;
        free(textBlock);
        //printf("New MainBlock length: %zu.\n", mainBlock->dataBlockCnt);
    }

    system("rm -rf tmp");
    fclose(file);
}

void printTextBlock(struct MainBlock *mainBlock, size_t textBlockIndex) {
    if (mainBlock == NULL) {
        printf("MainBlock required.\n");
        return;
    }
    printf("print %zu text block:\n", textBlockIndex);
    TextBlock *textBlock = (mainBlock->textBlocks + textBlockIndex);
    if (textBlock == NULL || textBlockIndex >= mainBlock->dataBlockCnt) {
        printf("TextBlock does not exists.\n");
        return;
    }
    printf("lines: %zu\n", textBlock->lines);
    char **firstLine = textBlock->content;
    for (size_t i = 0; i < textBlock->lines; i++) {
        printf("%zu: %s", i, *(firstLine + i));
        //pointer to the string
    }

}

void deleteTextBlock(struct MainBlock *mainBlock, size_t textBlockIndex) {
    if (mainBlock) {
        TextBlock *textBlock = (mainBlock->textBlocks + textBlockIndex);
        if (textBlock->content) {
            char **firstLine = textBlock->content;
            for (int i = 0; i < textBlock->lines; i++) {
                if (*(firstLine + i)) free(*(firstLine + i));
            }
            free(textBlock->content);
            textBlock->content = NULL;
            textBlock->lines = 0;
        }
        mainBlock->dataBlockCnt--;
    }
}


void deleteMainBlock(MainBlock *mainBlock) {
    if (mainBlock) {
        for (size_t textBlockIndex = 0; textBlockIndex < mainBlock->mainBlockLimit; textBlockIndex++) {
            if (mainBlock->textBlocks + textBlockIndex)
                deleteTextBlock(mainBlock, textBlockIndex);
        }
        free(mainBlock);
    }
}

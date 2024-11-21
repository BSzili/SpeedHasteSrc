#include <llmouse.h>

bool LLM_Init(void) {
    return FALSE;
}

word LLM_GetState(int *x, int *y) {
        if (x != NULL)
            *x = 0;
        if (y != NULL)
            *y = 0;
        return 0;
}

word LLM_GetMovement(int *x, int *y) {
        if (x != NULL)
            *x = 0;
        if (y != NULL)
            *y = 0;
    return 0;
}


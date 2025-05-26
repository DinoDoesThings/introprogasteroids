#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <float.h>

// custom headers
#include "typedef.h"
#include "config.h"


bool checkCollision(GameObject* a, GameObject* b) {
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    float distance = sqrt(dx * dx + dy * dy);
    
    return distance < (a->radius + b->radius);
}

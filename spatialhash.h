#ifndef SPAITIALHASH_H
#define SPAITIALHASH_H
#include <stdbool.h>
#include <list.h>
#include <geometry.h>

LIST_DEC(size_t)


typedef struct hash_point{
    size_t x,y;
}hash_point;

typedef struct bucket{
    LIST(size_t) rect_indexes;
    rect box;
}bucket;

typedef struct location{
    hash_point pos;
    size_t index_in_bucket;
}location;

LIST_DEC(location)

typedef struct rect_handler{
    rect rect;
    LIST(location) refrences;
}rect_handler;

LIST_DEC(rect_handler)

typedef struct spatialhash{
    size_t grid_width;
    size_t grid_height;
    float bucket_dim;
    bucket** buckets;
    LIST(rect_handler) unique_rects;
}spatialhash;

typedef struct collision{
    size_t first;
    size_t second;
}collision;

LIST_DEC(collision)

bool aabb(rect rect1,rect rect2);
bool test_point(vec2 point,rect rect);
spatialhash create_spatialhash(float map_width,float map_height,float bucket_dim);
void destroy_spatialhash(spatialhash* spatialhash);
size_t add_rect(spatialhash* spatialhash,rect rect);
bool update_rect(spatialhash* spatialhash,size_t index,rect rect);
LIST(collision) get_collisions(spatialhash* spatialhash);
LIST(size_t) search_collisions(spatialhash* spatialhash,rect rect);
LIST(size_t) search_point(spatialhash* spatialhash,vec2 pos);
bool delete_rect(spatialhash* spatialhash,size_t index);
#endif

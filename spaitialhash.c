#include <list.h>
#include <math.h>
#include <stdio.h>
#define FOR(i,length) int i;for(i=0;i<length;i++)
#define FOR_CUSTOM(i,a,c,length) int i;for(i=a;i<length;i += c)
#define ARRAY(type,number) malloc(sizeof(type)*number)
#define PRINT_NUMBER(name, number) printf("%s: %d\n",name, number)
#define PRINT_INDEX(name, index) printf("%s: %lu\n",name, index)

typedef struct vec2{
    float x;
    float y;
}vec2;
vec2 create_vec2(float x,float y){
    vec2 vec = {x,y};
    return vec;
}
typedef struct rect{
    vec2 center;
    vec2 half_dim;
}rect;

static size_t id_counter = 0;

rect create_rect(float x,float y,float width,float height){
    rect output = {create_vec2(x,y),create_vec2(width/2,height/2)};
    return output;
}

bool aabb(rect rect1,rect rect2){
    return (fabsf(rect1.center.x-rect2.center.x) < (rect1.half_dim.x + rect2.half_dim.x) && fabsf(rect1.center.y-rect2.center.y) < (rect1.half_dim.y + rect2.half_dim.y));
}

typedef struct hash_point{
    size_t x,y;
}hash_point;

LIST_DEC(size_t)
LIST_DEF(size_t)
typedef struct bucket{
    LIST(size_t) rect_indexes;
    rect box;
}bucket;

bucket create_bucket(){
    bucket output = {LIST_CREATE(size_t,10)};
    return output;
}

void destroy_bucket(bucket* bucket){
    LIST_DESTROY(size_t,&bucket->rect_indexes);
}

typedef struct location{
    hash_point pos;
    size_t index_in_bucket;
}location;

LIST_DEC(location)
LIST_DEF(location)
typedef struct rect_handler{
    rect rect;
    LIST(location) refrences;
}rect_handler;

LIST_DEC(rect_handler)
LIST_DEF(rect_handler)
typedef struct spatialhash{
    size_t grid_width;
    size_t grid_height;
    float bucket_dim;
    bucket** buckets;
    LIST(rect_handler) unique_rects;
}spatialhash;

spatialhash create_spatialhash(float map_width,float map_height,float bucket_dim){
    spatialhash output;
    output.grid_width = map_width/bucket_dim;
    output.grid_height = map_width/bucket_dim;
    if(output.grid_width == 0)
        output.grid_width = 1;
    if(output.grid_height == 0)
        output.grid_height = 1;
    
    output.bucket_dim = bucket_dim;
    output.buckets = ARRAY(bucket*,output.grid_height);
    FOR(y,output.grid_height){
        output.buckets[y]  = ARRAY(bucket,output.grid_width);
        FOR(x,output.grid_width){
            output.buckets[y][x] = create_bucket();
        }
    }
    output.unique_rects = LIST_CREATE(rect_handler,1000);
    return output;
}

void destroy_spatialhash(spatialhash* spatialhash){
    FOR(y,spatialhash->grid_height){
        FOR(x,spatialhash->grid_width){
            destroy_bucket(&spatialhash->buckets[y][x]);
        }
        free(spatialhash->buckets[y]);
    }
    free(spatialhash->buckets);
    FOR(i,spatialhash->unique_rects.item_size){
        LIST_DESTROY(location,&spatialhash->unique_rects.items[i].data.refrences);
    }
    LIST_DESTROY(rect_handler,&spatialhash->unique_rects);
}

hash_point hash(spatialhash* spatialhash,vec2 pos){
    int x = (pos.x)/spatialhash->bucket_dim+(spatialhash->grid_width/2);
    int y = (pos.y)/spatialhash->bucket_dim + (spatialhash->grid_height/2);
    if(x < 0)
        x = 0;
    if(y < 0)
        y=0;
    if(x > spatialhash->grid_width-1)
        x = spatialhash->grid_width-1;
    if(y > spatialhash->grid_height-1)
        y = spatialhash->grid_height-1;
    hash_point point = {x,y};
    return point;
}

size_t add_rect(spatialhash* spatialhash,rect rect){
    hash_point bottom_left = hash(spatialhash,create_vec2(rect.center.x-rect.half_dim.x,rect.center.y-rect.half_dim.y));
    hash_point top_right = hash(spatialhash,create_vec2(rect.center.x+rect.half_dim.x,rect.center.y+rect.half_dim.y));
    int max_x = top_right.x+1;
    int max_y = top_right.y+1;
    rect_handler handler;
    handler.rect = rect;
    handler.refrences = LIST_CREATE(location,10);
    size_t index = LIST_ADD(rect_handler,&spatialhash->unique_rects,handler);
    FOR_CUSTOM(y,bottom_left.y,1,max_y){
        FOR_CUSTOM(x,bottom_left.x,1,max_x){
            size_t index_in_bucket = LIST_ADD(size_t,&spatialhash->buckets[y][x].rect_indexes,index);
            hash_point point = {x,y};
            location loc = {point,index_in_bucket};
            LIST_ADD(location,&handler.refrences,loc);
        }
    }
    *LIST_GET(rect_handler,&spatialhash->unique_rects,index) = handler;
    return index;
}

bool update_rect(spatialhash* spatialhash,size_t index,rect rect){
    int i=0;
    rect_handler* handler = LIST_GET(rect_handler,&spatialhash->unique_rects,index);
    if(handler){ 
        handler->rect = rect;
        FOR(i,handler->refrences.item_size){
            location loc = handler->refrences.items[i].data;
            LIST_DELETE(size_t,&spatialhash->buckets[loc.pos.y][loc.pos.x].rect_indexes,loc.index_in_bucket);
            LIST_DELETE(location,&handler->refrences,handler->refrences.items[i].handle_index);
        }
        hash_point bottom_left = hash(spatialhash,create_vec2(rect.center.x-rect.half_dim.x,rect.center.y-rect.half_dim.y));
        hash_point top_right = hash(spatialhash,create_vec2(rect.center.x+rect.half_dim.x,rect.center.y+rect.half_dim.y));
        int max_y = top_right.y;
        int max_x = top_right.x;
        FOR_CUSTOM(y,bottom_left.y,1,max_y){
            FOR_CUSTOM(x,bottom_left.x,1,max_x){
                size_t index_in_bucket = LIST_ADD(size_t,&spatialhash->buckets[y][x].rect_indexes,index);
                hash_point point = {x,y};
                location loc = {point,index_in_bucket};
                LIST_ADD(location,&handler->refrences,loc);
            }
        }
        return true;
    }
    return false;
}

typedef struct collision{
    size_t first;
    size_t second;
}collision;

LIST_DEC(collision)
LIST_DEF(collision)
LIST(collision) get_collisions(spatialhash* spatialhash){
    LIST(collision) output = LIST_CREATE(collision,10);
    FOR(y,spatialhash->grid_height){
        FOR(x,spatialhash->grid_width){
            bucket* test = &spatialhash->buckets[y][x];
            FOR(i,test->rect_indexes.item_size){
                FOR(j,test->rect_indexes.item_size){
                    if(i != j){
                        rect_handler* first_rect = LIST_GET(rect_handler,&spatialhash->unique_rects,test->rect_indexes.items[i].data);
                        rect_handler* second_rect = LIST_GET(rect_handler,&spatialhash->unique_rects,test->rect_indexes.items[j].data);
                        size_t index_of_first_rect = test->rect_indexes.items[i].data;
                        size_t index_of_second_rect = test->rect_indexes.items[j].data;
                        if(first_rect && second_rect && aabb(first_rect->rect,second_rect->rect)){
                            bool unique_collision = true;
                            FOR(z,output.item_size){
                                if(output.items[z].data.first == index_of_first_rect && output.items[z].data.second == index_of_second_rect){
                                    unique_collision = false;
                                }
                            }
                            if(unique_collision){
                                collision new_collision = {index_of_first_rect,index_of_second_rect};
                                LIST_ADD(collision,&output,new_collision);
                            }
                        }
                    }
                }
            }
        }
    }
    return output;
}



int main(){
    spatialhash hash = create_spatialhash(20,20,10);
    rect rect1 = create_rect(2.5,2.5,5,5);
    rect rect2 = create_rect(3.5,2.5,5,5);
    rect rect3 = create_rect(0,1,3,3);
    size_t index1 = add_rect(&hash,rect1);
    size_t index2 = add_rect(&hash,rect2);
    size_t index3 = add_rect(&hash,rect3);
    rect new_rect = create_rect(-2.5,0,5,1);
    update_rect(&hash,index2,new_rect);
    LIST(collision) collisions = get_collisions(&hash);
    FOR(i,collisions.item_size){
        printf("Collision at :%lu,%lu\n",collisions.items[i].data.first,collisions.items[i].data.second);
    }
    destroy_spatialhash(&hash);
}



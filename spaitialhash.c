#include <list.h>
#include <math.h>
#define FOR(i,length) for(;i<length;i++)
#define ARRAY(type,number) malloc(sizeof(type)*number)
const float CELL_DIMENSION = 1;
const size_t MAP_WIDTH = 10;
const size_t MAP_HEIGHT = 10;
const size_t BUCKET_INIT_SIZE = 10;

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

rect create_rect(vec2 center,vec2 half_dim){
    rect output = {center,half_dim};
    return output;
}

bool aabb(rect rect1,rect rect2){
    return (fabsf(rect1.center.x-rect2.center.x) < (rect1.half_dim.x + rect2.half_dim.y) && fabsf(rect1.center.y-rect2.center.y) < (rect1.half_dim.y + rect2.half_dim.y));
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

bucket create_bucket(rect box){
    bucket output = {LIST_CREATE(size_t,BUCKET_INIT_SIZE),box};
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
    int y=0,x=0;
    spatialhash output;
    output.grid_width = map_width/bucket_dim;
    output.grid_height = map_width/bucket_dim;
    output.bucket_dim = bucket_dim;
    output.buckets = ARRAY(bucket*,output.grid_height);
    FOR(y,output.grid_height){
        output.buckets[y]  = ARRAY(bucket,output.grid_width);
        FOR(x,output.grid_width){
            output.buckets[y][x] = create_bucket(create_rect(create_vec2(x,y),create_vec2(bucket_dim/2,bucket_dim/2)));
        }
    }
    output.unique_rects = LIST_CREATE(rect_handler,1000);
    return output;
}

void delete_spatialhash(spatialhash* spatialhash){
    int y=0,x=0;
    FOR(y,spatialhash->grid_height){
        FOR(x,spatialhash->grid_width){
            destroy_bucket(&spatialhash->buckets[y][x]);
        }
        free(spatialhash->buckets[y]);
    }
    free(spatialhash->buckets);
    int i=0;
    FOR(i,spatialhash->unique_rects.item_size){
        LIST_DESTROY(location,&spatialhash->unique_rects.items[i].data.refrences);
    }
    LIST_DESTROY(rect_handler,&spatialhash->unique_rects);
}

hash_point hash(spatialhash* spatialhash,vec2 pos){
    int x = (pos.x)/spatialhash->bucket_dim+(spatialhash->grid_width/2);
    int y = (pos.y)/CELL_DIMENSION + (spatialhash->grid_height/2);
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
    int x = bottom_left.x;
    int y = bottom_left.y;
    hash_point top_right = hash(spatialhash,create_vec2(rect.center.x+rect.half_dim.x,rect.center.y+rect.half_dim.y));
    int max_x = top_right.x;
    int max_y = top_right.y;
    rect_handler handler;
    size_t index = LIST_ADD(rect_handler,&spatialhash->unique_rects,handler);
    FOR(y,max_y){
        FOR(x,max_x){
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
        FOR(i,handler->refrences.item_size){
            location loc = handler->refrences.items[i].data;
            LIST_DELETE(size_t,&spatialhash->buckets[loc.pos.y][loc.pos.x].rect_indexes,loc.index_in_bucket);
            LIST_DELETE(location,&handler->refrences,handler->refrences.items[i].handle_index);
        }
        hash_point bottom_left = hash(spatialhash,create_vec2(rect.center.x-rect.half_dim.x,rect.center.y-rect.half_dim.y));
        int x = bottom_left.x;
        int y = bottom_left.y;
        hash_point top_right = hash(spatialhash,create_vec2(rect.center.x+rect.half_dim.x,rect.center.y+rect.half_dim.y));
        int max_x = top_right.x;
        int max_y = top_right.y;
        FOR(y,max_y){
            FOR(x,max_x){
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
    int y=0,x=0;
    LIST(collision) output = LIST_CREATE(collision,10);
    FOR(y,spatialhash->grid_height){
        FOR(x,spatialhash->grid_width){
            bucket* test = &spatialhash->buckets[y][x];
            int i=0,j=0;
            FOR(i,test->rect_indexes.item_size){
                FOR(j,test->rect_indexes.item_size){
                    if(i != j){
                        rect_handler* first_rect = LIST_GET(rect_handler,&spatialhash->unique_rects,test->rect_indexes.items[i].data);
                        rect_handler* second_rect = LIST_GET(rect_handler,&spatialhash->unique_rects,test->rect_indexes.items[j].data);
                        if(first_rect && second_rect && aabb(first_rect->rect,second_rect->rect)){
                            collision new_collision = {test->rect_indexes.items[i].data,test->rect_indexes.items[j].data};
                            LIST_ADD(collision,&output,new_collision);
                        }
                    }
                }
            }
        }
    }
    return output;
}



int main(){
    spatialhash hash = create_spatialhash(1000,1000,1);

}



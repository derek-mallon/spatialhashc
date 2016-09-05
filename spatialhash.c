#include <math.h>
#define FOR(i,length) int i;for(i=0;i<length;i++)
#define FOR_CUSTOM(i,a,c,length) int i;for(i=a;i<length;i += c)
#define PRINT_NUMBER(name, number) printf("%s: %d\n",name, number)
#define PRINT_INDEX(name, index) printf("%s: %lu\n",name, index)
#include "spatialhash.h"
LIST_DEF(size_t)
LIST_DEF(collision)
LIST_DEF(location)
LIST_DEF(rect_handler)

vec2 create_vec2(float x,float y){
    vec2 vec = {x,y};
    return vec;
}
rect create_rect(float x,float y,float width,float height){
    rect output = {create_vec2(x,y),create_vec2(width/2,height/2)};
    return output;
}

bool aabb(rect rect1,rect rect2){
    return (fabsf(rect1.center.x-rect2.center.x) < (rect1.half_dim.x + rect2.half_dim.x) && fabsf(rect1.center.y-rect2.center.y) < (rect1.half_dim.y + rect2.half_dim.y));
}

bool test_point(vec2 point,rect rect){
    return (fabsf(rect.center.x-point.x) < (rect.half_dim.x) && fabsf(rect.center.y-point.y) < (rect.half_dim.y));
}



bucket create_bucket(){
    bucket output = {LIST_CREATE(size_t,10)};
    return output;
}

void destroy_bucket(bucket* bucket){
    LIST_DESTROY(size_t,&bucket->rect_indexes);
}



spatialhash create_spatialhash(float map_width,float map_height,float bucket_dim){
    spatialhash output;
    output.grid_width = map_width/bucket_dim;
    output.grid_height = map_width/bucket_dim;
    if(output.grid_width == 0)
        output.grid_width = 1;
    if(output.grid_height == 0)
        output.grid_height = 1;
    output.bucket_dim = bucket_dim;
    output.buckets = malloc(sizeof(bucket*) * output.grid_height);
    FOR(y,output.grid_height){
        output.buckets[y]  = malloc(sizeof(bucket)* output.grid_width);
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
    rect_handler handler;
    handler.rect = rect;
    handler.refrences = LIST_CREATE(location,10);
    size_t index = LIST_ADD(rect_handler,&spatialhash->unique_rects,handler);
    FOR_CUSTOM(y,bottom_left.y,1,top_right.y+1){
        FOR_CUSTOM(x,bottom_left.x,1,top_right.x+1){
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
        FOR_CUSTOM(y,bottom_left.y,1,top_right.y+1){
            FOR_CUSTOM(x,bottom_left.x,1,top_right.x+1){
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

LIST(size_t) search_collisions(spatialhash* spatialhash,rect rect){
    LIST(size_t) output = LIST_CREATE(size_t,10);
    hash_point bottom_left = hash(spatialhash,create_vec2(rect.center.x-rect.half_dim.x,rect.center.y-rect.half_dim.y));
    hash_point top_right = hash(spatialhash,create_vec2(rect.center.x+rect.half_dim.x,rect.center.y+rect.half_dim.y));
    FOR_CUSTOM(y,bottom_left.y,1,top_right.y+1){
        FOR_CUSTOM(x,bottom_left.x,1,top_right.x+1){
            bucket* test = &spatialhash->buckets[y][x];
            FOR(i,test->rect_indexes.item_size){
                rect_handler* test_rect = LIST_GET(rect_handler,&spatialhash->unique_rects,test->rect_indexes.items[i].data);
                if(test_rect && aabb(rect,test_rect->rect)){
                    bool unique_collision = true;
                    FOR(j,output.item_size){
                        if(output.items[j].data == test->rect_indexes.items[i].data){
                            unique_collision = false;
                        }
                    }
                    if(unique_collision){
                        LIST_ADD(size_t,&output,test->rect_indexes.items[i].data);
                    }
                }
            }
        }
    }
    return output;
}

LIST(size_t) search_point(spatialhash* spatialhash,vec2 pos){
    LIST(size_t) output = LIST_CREATE(size_t,10);
    hash_point point = hash(spatialhash,pos);
    bucket* test = &spatialhash->buckets[point.y][point.x];
    FOR(i,test->rect_indexes.item_size){
        rect_handler* test_rect = LIST_GET(rect_handler,&spatialhash->unique_rects,test->rect_indexes.items[i].data);
        if(test_rect && test_point(pos,test_rect->rect)){
            bool unique_collision = true;
            FOR(j,output.item_size){
                if(output.items[j].data == test->rect_indexes.items[i].data){
                    unique_collision = false;
                }
            }
            if(unique_collision){
                LIST_ADD(size_t,&output,test->rect_indexes.items[i].data);
            }
        }
    }
    return output;
}

bool delete_rect(spatialhash* spatialhash,size_t index){
    rect_handler* handler = LIST_GET(rect_handler,&spatialhash->unique_rects,index);
    if(handler){ 
        FOR(i,handler->refrences.item_size){
            location loc = handler->refrences.items[i].data;
            LIST_DELETE(size_t,&spatialhash->buckets[loc.pos.y][loc.pos.x].rect_indexes,loc.index_in_bucket);
            LIST_DELETE(location,&handler->refrences,handler->refrences.items[i].handle_index);
        }
        LIST_DELETE(rect_handler,&spatialhash->unique_rects,index);
        return true;
    }
    return false;
}

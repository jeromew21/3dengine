#include "SDL.h"
void print(char* o) { printf(o); printf("\n"); }

#define WIDTH 1280
#define HEIGHT 720
#define focal_length 1000.0
#define padding_left 640
#define padding_bottom 360


typedef struct Vector3 {
    double x;
    double y;
    double z;
} Vector3;

typedef struct Vertex {
    Vector3 absolute_position;
    Vector3 local_transform;
    Vector3 perspective;
} Vertex;

typedef struct Polygon {
    Vertex* sequence;
    int num_vertices;
    int vertices_added;
} Polygon;

void matrix_x_matrix(double m1[][3], double m2[][3], double result[][3]) {
    int i, j, k;
    for(i = 0; i < 3; i++) {
        for(j = 0; j < 3; j++) {
            for(k = 0; k < 3; k++) {
                result[i][j] += m1[i][k] * m2[k][j];
            }
        }
    }
}

double dot_product(double row[3], Vector3 v) {
    return row[0]*v.x + row[1]*v.y + row[2]*v.z;
}

Vector3 matrix_x_vector(double m[][3], Vector3 v) {
    Vector3 vect;
    vect.x = dot_product(m[0], v);
    vect.y = dot_product(m[1], v);
    vect.z = dot_product(m[2], v);
    return vect;
}

int x2d(Vector3* v) {
    return padding_left + v->x * (focal_length/(v->z+focal_length));
}

int y2d(Vector3* v) {
    return padding_bottom + v->y * (focal_length/(v->z+focal_length));
}

void print_vec(Vector3 v) {
    printf("x: %f y: %f z: %f\n", v.x, v.y, v.z);
}

Polygon* create_polygon(int num_vertices) {
    Polygon* poly = malloc(sizeof(Polygon));
    poly->sequence = malloc(sizeof(Vertex)*num_vertices);
    poly->num_vertices = num_vertices;
    poly->vertices_added = 0;
    return poly;
}

void push_vertex(Polygon* poly, double x, double y, double z) {
    //printf("Inserting at location %i\n", poly->vertices_added);
    //memcpy(poly->sequence + poly->vertices_added, v, sizeof(Vector3));
    Vector3 pos; pos.x = x; pos.y = y; pos.z = z;
    Vertex vertex;
    vertex.absolute_position = pos;
    vertex.local_transform = pos;
    vertex.perspective = pos;
    poly->sequence[poly->vertices_added] = vertex;
    //print_vec(poly->sequence[poly->vertices_added]);
    poly->vertices_added += 1;
}

void render_polygon(SDL_Renderer* renderer, Polygon* polygon) {
    Vertex* p = polygon->sequence;
    Vector3 vect;
    Vector3 last = (*p).perspective;
    Vector3 first = (*p).perspective;
    int i = 0;
    while (i < polygon->num_vertices) {
        vect = (*p).perspective;
        SDL_RenderDrawLine(renderer, x2d(&last), y2d(&last), x2d(&vect), y2d(&vect));
        last = vect;
        p++; i++;
    }
    SDL_RenderDrawLine(renderer, x2d(&vect), y2d(&vect), x2d(&first), y2d(&first));
}

int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) == 0) {
        SDL_Window* window = NULL;
        SDL_Renderer* renderer = NULL;

        Polygon* polygon1 = create_polygon(4);
        push_vertex(polygon1, 100, 100, 100);
        push_vertex(polygon1, 100, 200, 100);
        push_vertex(polygon1, 200, 200, 100);
        push_vertex(polygon1, 200, 100, 100);

        Polygon* xaxis = create_polygon(2);
        push_vertex(xaxis, 0, 0, 0);
        push_vertex(xaxis, 0, 0, 300);
        Polygon* yaxis = create_polygon(2);
        push_vertex(yaxis, 0, 0, 0);
        push_vertex(yaxis, 0, 300, 0);
        Polygon* zaxis = create_polygon(2);
        push_vertex(zaxis, 0, 0, 0);
        push_vertex(zaxis, 0, 0, 300);


        if (SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, 0, &window, &renderer) == 0) {
            print("Window created. Starting game loop...");
            SDL_bool done = SDL_FALSE;

            while (!done) {
                SDL_Event event;

                SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
                SDL_RenderClear(renderer);

                SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
                
                render_polygon(renderer, xaxis);
                render_polygon(renderer, yaxis);
                render_polygon(renderer, zaxis);
                render_polygon(renderer, polygon1);

                SDL_RenderPresent(renderer);

                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_QUIT) {
                        done = SDL_TRUE;
                    }
                }
            }
        }

        if (renderer) {
            SDL_DestroyRenderer(renderer);
        }
        if (window) {
            SDL_DestroyWindow(window);
        }
    }
    SDL_Quit();
    return 0;
}

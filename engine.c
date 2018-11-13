#include <stdio.h>
#include <math.h>

#include "SDL.h"
#include "SDL_ttf.h"

#define WIDTH 1280
#define HEIGHT 720
#define focal_length 5000.0
#define padding_left 640
#define padding_bottom 0

//For movement, do not change translations of points, change camera/padding

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
    SDL_Color color;
} Polygon;

typedef struct Mesh {
    Polygon** polygons; //List of pointers to polygons
    int num_polygons;
    int polygons_added;
    int is_valid;
    Vector3 center;
    Vector3 rotation;
} Mesh;

typedef struct World {
    Mesh** meshes; //List of pointers to meshes
    int num_meshes;
    int meshes_added;
} World;

void print(char* o) { printf(o); printf("\n"); }

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

void print_vec(Vector3 v) {
    printf("x: %f y: %f z: %f\n", v.x, v.y, v.z);
}

int x2d(Vector3* v, Vector3* t) {
    return padding_left + (v->x+(-1*t->x))*(focal_length/((v->z+(t->z-focal_length))+focal_length));
}

int y2d(Vector3* v, Vector3* t) {
    return HEIGHT - (padding_bottom + (v->y+t->y)*(focal_length/((v->z+(t->z-focal_length))+focal_length)));
}

Polygon* create_polygon(int num_vertices, SDL_Color* color) {
    Polygon* poly = malloc(sizeof(Polygon));
    poly->sequence = malloc(sizeof(Vertex)*num_vertices);
    poly->num_vertices = num_vertices;
    poly->vertices_added = 0;
    poly->color = *color;
    return poly;
}

Mesh* create_mesh(int num_polygons) {
    Mesh* mesh = malloc(sizeof(Mesh));
    mesh->polygons = malloc(sizeof(Polygon*)*num_polygons);
    mesh->num_polygons = num_polygons;
    mesh->polygons_added = 0;
    mesh->is_valid = 1;
    mesh->center.x = 0; mesh->center.y = 0; mesh->center.y = 0;
    mesh->rotation.x = 0; mesh->rotation.y = 0; mesh->rotation.y = 0;
    return mesh;
}

World* create_world(int num_meshes) {
    World* world = malloc(sizeof(World));
    world->meshes = malloc(sizeof(Mesh*)*num_meshes + 2);
    world->num_meshes = num_meshes;
    world->meshes_added = 0;
    int i;
    for (i = 0; i < num_meshes; i++) {
        world->meshes[i] = NULL;
    }
    return world;
}

void rotate_mesh(Mesh* mesh) { //Affects local_transform
    Vector3* rotation = &(mesh->rotation);
    Vector3 center = mesh->center;
    double rx[3][3] = {
        {1, 0, 0},
        {0, cos(rotation->x), -1*sin(rotation->x)},
        {0, sin(rotation->x), cos(rotation->x)}
    };
    double ry[3][3] = {
        {cos(rotation->y), 0, sin(rotation->y)},
        {0, 1, 0},
        {-1*sin(rotation->y), 0, cos(rotation->y)}
    };
    double rz[3][3] = {
        {cos(rotation->z), -1*sin(rotation->z), 0},
        {sin(rotation->z), cos(rotation->z), 0},
        {0, 0, 1}
    };
    double res1[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
    matrix_x_matrix(ry, rx, res1);
    double transform[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
    matrix_x_matrix(rz, res1, transform);

    Vertex* vert;
    Vector3 vect;
    int i, k;
    for (i = 0; i < mesh->num_polygons; i++) {
        for (k = 0; k < mesh->polygons[i]->num_vertices; k++) {
            vert = &(mesh->polygons[i]->sequence[k]);
            vect.x = vert->absolute_position.x - center.x,
            vect.y = vert->absolute_position.y - center.y,
            vect.z = vert->absolute_position.z - center.z,
            vert->local_transform = matrix_x_vector(transform, vect);
            vert->local_transform.x += center.x;
            vert->local_transform.y += center.y;
            vert->local_transform.z += center.z;
        }
    }
}

void rotate_all_in_world(World* world, Vector3 axes, Vector3 origin) { //affects perspective
    double rx[3][3] = {
        {1, 0, 0},
        {0, cos(axes.x), -1*sin(axes.x)},
        {0, sin(axes.x), cos(axes.x)}
    };
    double ry[3][3] = {
        {cos(axes.y), 0, sin(axes.y)},
        {0, 1, 0},
        {-1*sin(axes.y), 0, cos(axes.y)}
    };
    double rz[3][3] = {
        {cos(axes.z), -1*sin(axes.z), 0},
        {sin(axes.z), cos(axes.z), 0},
        {0, 0, 1}
    };

    double res1[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
    matrix_x_matrix(ry, rx, res1);
    double transform[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
    matrix_x_matrix(rz, res1, transform);

    int i, j, k;
    Vertex* vertex;
    Vector3 vect;
    for (i = 0; i < world->meshes_added; i++) {
        rotate_mesh(world->meshes[i]);
        for (j = 0; j < world->meshes[i]->num_polygons; j++) {
            for (k = 0; k < world->meshes[i]->polygons[j]->num_vertices; k++) {
                vertex = &(world->meshes[i]->polygons[j]->sequence[k]);
                vect.x = vertex->local_transform.x - origin.x,
                vect.y = vertex->local_transform.y - origin.y,
                vect.z = vertex->local_transform.z - origin.z,
                vertex->perspective = matrix_x_vector(transform, vect);
                vertex->perspective.x += origin.x;
                vertex->perspective.y += origin.y;
                vertex->perspective.z += origin.z;
            }
        }
    }
}

//Add a vertex to a polygon
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

//Add Polygon to a mesh
void add_polygon(Mesh* mesh, Polygon* poly) {
    mesh->polygons[mesh->polygons_added] = poly;
    mesh->polygons_added += 1;
}


//Add mesh to world
void add_mesh(World* world, Mesh* mesh) {
    world->meshes[world->meshes_added] = mesh;
    world->meshes_added += 1;
}


//Render a polygon
void render_polygon(SDL_Renderer* renderer, Polygon* polygon, Vector3* translation) { //Add translations here
    //Shading happens here
    Vertex* p = polygon->sequence;
    Vector3 vect;
    Vector3 last = (*p).perspective;
    Vector3 first = (*p).perspective;
    int i = 0;
    while (i < polygon->num_vertices) {
        vect = (*p).perspective;
        SDL_RenderDrawLine(renderer, x2d(&last, translation), y2d(&last, translation), x2d(&vect, translation), y2d(&vect, translation));
        last = vect;
        p++; i++;
    }
    SDL_RenderDrawLine(renderer, x2d(&vect, translation), y2d(&vect, translation), x2d(&first, translation), y2d(&first, translation));
}

//Render each of a mesh's polygons
void render_mesh(SDL_Renderer* renderer, Mesh* mesh, Vector3* translation) {
    Polygon** p = mesh->polygons;
    int i = 0;
    while (i < mesh->num_polygons) {
        //Render polygons in order.
        render_polygon(renderer, *p, translation); //is pointer to a polygon
        p++; i++;
    }
}

//Render each mesh in a world
void render_world(SDL_Renderer* renderer, World* world, Vector3* translation) {
    Mesh** p = world->meshes;
    int i = 0;
    while (i < world->meshes_added) {
        //Render meshes in order.
        //if (*p == NULL) { break; }        
        render_mesh(renderer, *p, translation);
        i++; p++;
    }
}

void free_world(World* world) {
    int i, j;
    for (i = 0; i < world->meshes_added; i++) {
        for (j = 0; j < world->meshes[i]->polygons_added; j++) {
            free(world->meshes[i]->polygons[j]);
        }
        free(world->meshes[i]);
    }
    free(world);
    print("Freed world");
}

Mesh* create_cube_mesh(int x, int y, int z, int w, int h, int l, SDL_Color* color) {
    int left = x - w/2;
    int right = x + w/2;
    int top = y + h/2;
    int bot = y - h/2;
    int front = z + l/2;
    int back = z - l/2;

    Polygon* polygon1 = create_polygon(4, color);
    push_vertex(polygon1, left, top, front);
    push_vertex(polygon1, right, top, front);
    push_vertex(polygon1, right, bot, front);
    push_vertex(polygon1, left, bot, front);

    Polygon* polygon2 = create_polygon(4, color);
    push_vertex(polygon2, left, top, back);
    push_vertex(polygon2, right, top, back);
    push_vertex(polygon2, right, bot, back);
    push_vertex(polygon2, left, bot, back);

    Polygon* polygon3 = create_polygon(4, color);
    push_vertex(polygon3, left, top, front);
    push_vertex(polygon3, left, top, back);
    push_vertex(polygon3, left, bot, back);
    push_vertex(polygon3, left, bot, front);

    Polygon* polygon4 = create_polygon(4, color);
    push_vertex(polygon4, right, top, front);
    push_vertex(polygon4, right, top, back);
    push_vertex(polygon4, right, bot, back);
    push_vertex(polygon4, right, bot, front);

    Polygon* polygon5 = create_polygon(4, color);
    push_vertex(polygon5, left, top, front);
    push_vertex(polygon5, left, top, back);
    push_vertex(polygon5, right, top, back);
    push_vertex(polygon5, right, top, front);

    Polygon* polygon6 = create_polygon(4, color);
    push_vertex(polygon6, left, bot, front);
    push_vertex(polygon6, left, bot, back);
    push_vertex(polygon6, right, bot, back);
    push_vertex(polygon6, right, bot, front);

    Mesh* cube = create_mesh(6);
    cube->center.x = x;
    cube->center.y = y;
    cube->center.z = z;
    add_polygon(cube, polygon1);
    add_polygon(cube, polygon2);
    add_polygon(cube, polygon3);
    add_polygon(cube, polygon4);
    add_polygon(cube, polygon5);
    add_polygon(cube, polygon6);

    return cube;
}

Mesh* create_axes_mesh() {
    SDL_Color color = { 255, 255, 255 };
    Polygon* x_axis = create_polygon(2, &color);
    push_vertex(x_axis, 0, 0, 0);
    push_vertex(x_axis, 300, 0, 0);
    Polygon* y_axis = create_polygon(2, &color);
    push_vertex(y_axis, 0, 0, 0);
    push_vertex(y_axis, 0, 300, 0);
    Polygon* z_axis = create_polygon(2, &color);
    push_vertex(z_axis, 0, 0, 0);
    push_vertex(z_axis, 0, 0, 300);

    Mesh* axes = create_mesh(3);
    add_polygon(axes, x_axis);
    add_polygon(axes, y_axis);
    add_polygon(axes, z_axis);

    return axes;
}

int main(int argc, char* argv[]) {
    int FRAME_LIMIT = 1000/300;
    int move_speed = 10;
    SDL_Color white = { 255, 255, 255 };        
    Vector3 zero; zero.x = 0; zero.y = 0; zero.z = 0;

    if (SDL_Init(SDL_INIT_VIDEO) == 0) {
        SDL_Window* window = NULL;
        SDL_Renderer* renderer = NULL;

        if (SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, 0, &window, &renderer) == 0) {
            print("Window created. Setting up...");

            if (TTF_Init() < 0) {
                // Error handling code
                print("TTF init failed, exiting");
                exit(0);
            }

            TTF_Font* font = TTF_OpenFont("/usr/share/fonts/TTF/arial.ttf", 20);
            if (font == NULL) {
                print("Font is null, exiting");
                exit(0);
            }
            print("Fonts initialized");

            Mesh* axes = create_axes_mesh();

            Mesh* cube = create_cube_mesh(100, 100, 100, 400, 100, 100, &white);
            Mesh* cube1 = create_cube_mesh(200, 100, 100, 100, 400, 100, &white);
            Mesh* cube2 = create_cube_mesh(200, 100, 100, 100, 100, 400, &white);

            World* world = create_world(4);
            add_mesh(world, axes);
            add_mesh(world, cube);
            add_mesh(world, cube1);
            add_mesh(world, cube2);

            Vector3 subject_translation; subject_translation.x = 0; subject_translation.y = 0; subject_translation.z = 3000;
            Vector3 subject_rotation; subject_rotation.x = 0; subject_rotation.y = 0; subject_rotation.z = 0;

            Uint16 pixels[16*16] = {  // raw pixel data:
                0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
                0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
                0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
                0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
                0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
                0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
                0x0fff, 0x0aab, 0x0789, 0x0bcc, 0x0eee, 0x09aa, 0x099a, 0x0ddd,
                0x0fff, 0x0eee, 0x0899, 0x0fff, 0x0fff, 0x1fff, 0x0dde, 0x0dee,
                0x0fff, 0xabbc, 0xf779, 0x8cdd, 0x3fff, 0x9bbc, 0xaaab, 0x6fff,
                0x0fff, 0x3fff, 0xbaab, 0x0fff, 0x0fff, 0x6689, 0x6fff, 0x0dee,
                0xe678, 0xf134, 0x8abb, 0xf235, 0xf678, 0xf013, 0xf568, 0xf001,
                0xd889, 0x7abc, 0xf001, 0x0fff, 0x0fff, 0x0bcc, 0x9124, 0x5fff,
                0xf124, 0xf356, 0x3eee, 0x0fff, 0x7bbc, 0xf124, 0x0789, 0x2fff,
                0xf002, 0xd789, 0xf024, 0x0fff, 0x0fff, 0x0002, 0x0134, 0xd79a,
                0x1fff, 0xf023, 0xf000, 0xf124, 0xc99a, 0xf024, 0x0567, 0x0fff,
                0xf002, 0xe678, 0xf013, 0x0fff, 0x0ddd, 0x0fff, 0x0fff, 0xb689,
                0x8abb, 0x0fff, 0x0fff, 0xf001, 0xf235, 0xf013, 0x0fff, 0xd789,
                0xf002, 0x9899, 0xf001, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0xe789,
                0xf023, 0xf000, 0xf001, 0xe456, 0x8bcc, 0xf013, 0xf002, 0xf012,
                0x1767, 0x5aaa, 0xf013, 0xf001, 0xf000, 0x0fff, 0x7fff, 0xf124,
                0x0fff, 0x089a, 0x0578, 0x0fff, 0x089a, 0x0013, 0x0245, 0x0eff,
                0x0223, 0x0dde, 0x0135, 0x0789, 0x0ddd, 0xbbbc, 0xf346, 0x0467,
                0x0fff, 0x4eee, 0x3ddd, 0x0edd, 0x0dee, 0x0fff, 0x0fff, 0x0dee,
                0x0def, 0x08ab, 0x0fff, 0x7fff, 0xfabc, 0xf356, 0x0457, 0x0467,
                0x0fff, 0x0bcd, 0x4bde, 0x9bcc, 0x8dee, 0x8eff, 0x8fff, 0x9fff,
                0xadee, 0xeccd, 0xf689, 0xc357, 0x2356, 0x0356, 0x0467, 0x0467,
                0x0fff, 0x0ccd, 0x0bdd, 0x0cdd, 0x0aaa, 0x2234, 0x4135, 0x4346,
                0x5356, 0x2246, 0x0346, 0x0356, 0x0467, 0x0356, 0x0467, 0x0467,
                0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
                0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
                0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
                0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff
            };
            SDL_Surface *icon_surface;
            icon_surface = SDL_CreateRGBSurfaceFrom(pixels,16,16,16,16*2,0x0f00,0x00f0,0x000f,0xf000);
            SDL_SetWindowIcon(window, icon_surface);
            SDL_FreeSurface(icon_surface);
                        
            SDL_SetWindowTitle(window, "engine"); 
            SDL_bool done = SDL_FALSE;

            char fps_chars[200];
            SDL_Surface* textSurface = TTF_RenderText_Blended(font, "???? FPS x: ???? y: ???? z: ????", white);
            SDL_Texture* message = SDL_CreateTextureFromSurface(renderer, textSurface);
            SDL_Rect textLocation = { 0, 0, 0, 0 };
            textLocation.w = textSurface->w;
            textLocation.h = textSurface->h;

            #define FPS_INTERVAL 1.0 //seconds.
            Uint32 fps_lasttime = SDL_GetTicks(); //the last recorded time.
            Uint32 fps_current; //the current FPS.
            Uint32 fps_frames = 0; //frames passed since the last recorded fps. At the tick of a second this will capture the number of frames rendered during that second

            //Main game loop
            print("Starting game loop...");
            while (!done) {
                SDL_Event event;

                SDL_SetRenderDrawColor(renderer, 30, 30, 30, SDL_ALPHA_OPAQUE);
                SDL_RenderClear(renderer); //I think this just clears the screen

                SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE); //Set draw color to white

                rotate_all_in_world(world, subject_rotation, subject_translation); //Perform rotations based on subject location
                render_world(renderer, world, &subject_translation);

                SDL_RenderCopy(renderer, message, NULL, &textLocation); //Render fps display

                SDL_RenderPresent(renderer); //Not sure exactly what this does but it's important

                fps_frames++;
                if (fps_lasttime < SDL_GetTicks() - FPS_INTERVAL*1000) { //We have hit a second: now display the number of frames that were rendered during that second
                    fps_lasttime = SDL_GetTicks();
                    fps_current = fps_frames;
                    fps_frames = 0;
                    sprintf(fps_chars, "%d FPS x: %.2f y: %.2f z: %.2f", fps_current, subject_translation.x, subject_translation.y, subject_translation.z);
                    textSurface = TTF_RenderText_Blended(font, fps_chars, white);
                    message = SDL_CreateTextureFromSurface(renderer, textSurface);
                    textLocation.w = textSurface->w;
                    textLocation.h = textSurface->h;
                }

                while (SDL_PollEvent(&event)) {
                    switch(event.type) {
                        case SDL_QUIT:
                            done = SDL_TRUE;
                            break;
                        case SDL_KEYDOWN:
                            switch( event.key.keysym.sym ){
                                case SDLK_LEFT:
                                    print("Left");
                                    subject_translation.x -= move_speed; //Will need to implement move forward based on direction
                                    break;
                                case SDLK_RIGHT:
                                    print("Right");
                                    subject_translation.x += move_speed;
                                    break;
                                case SDLK_UP:
                                    print("Up");
                                    subject_translation.z -= move_speed;
                                    break;
                                case SDLK_DOWN:
                                    print("Down");
                                    subject_translation.z += move_speed;
                                    break;
                                case SDLK_a:
                                    print("a");
                                    subject_rotation.y -= 0.001;
                                    break;
                                case SDLK_d:
                                    print("d");
                                    subject_rotation.y += 0.001;
                                    break;
                                default:
                                    break;
                            }
                            break;
                        case SDL_KEYUP:
                            break;
                        default:
                            break;
                    }
                }
                //For now we want to see what the max framerate is, so comment out the delay
                //SDL_Delay(FRAME_LIMIT);
            } //end game loop
            print("Cleaning up..."); //hopefully this gets everything
            SDL_FreeSurface(textSurface);
            SDL_DestroyTexture(message);
            free_world(world);
            TTF_CloseFont(font);
            TTF_Quit();
        } //end if
        if (renderer) {
            SDL_DestroyRenderer(renderer);
        }
        if (window) {
            SDL_DestroyWindow(window);
        }
    } //end if
    SDL_Quit();
    print("Done");
    return 0;
}

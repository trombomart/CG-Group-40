#include <stdio.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#ifdef WIN32
#include <windows.h>
#endif
#include <limits>
#include "raytracing.h"


// temporary variables
// these are only used to illustrate
// a simple debug drawing. A ray
std::vector<std::vector<Vec3Df> > test_rays;


//use this function for any preprocessing of the mesh.
void init(std::string mesh_filename) {
    // load the mesh file
    // please realize that not all OBJ files will successfully load.
    // Nonetheless, if they come from Blender, they should, if they
    // are exported as WavefrontOBJ.
    // PLEASE ADAPT THE LINE BELOW TO THE FULL PATH OF THE dodgeColorTest.obj
    // model, e.g., "C:/temp/myData/GraphicsIsFun/dodgeColorTest.obj",
    // otherwise the application will not load properly
    MyMesh.loadMesh(mesh_filename.c_str(), true);
    MyMesh.computeVertexNormals();

    // one first move: initialize the first light source
    // at least ONE light source has to be in the scene!!!
    // here, we set it to the current location of the camera
    MyLightPositions.push_back(MyCameraPosition);
}

int maxLevel = 20;
Vec3Df backgroundColor = Vec3Df(1.f/255, 1.f/255, 1.f/255);
float EPSILON = 0.0001f;

/**
 * @source "How to get a reflection vector" http://math.stackexchange.com/questions/13261/how-to-get-a-reflection-vector 2015-06-11
 **/
const Vec3Df ComputeReflectedRay(const Vec3Df &ray_destination, const Vec3Df &normalized_normal) {
    return ray_destination - (2 * (Vec3Df::dotProduct(ray_destination, normalized_normal)) * normalized_normal);
}

void ComputeDirectLight(const Vec3Df & eye_hit, const int triangle_index, Vec3Df & direct_color) {

    double factor = 0.1;

    // for each light source
    for (unsigned int i = 0; i < MyLightPositions.size(); i++) {
        // Check if hitpoint is illumenated by this light
        Vec3Df light_origin = MyLightPositions[i];
        const Vec3Df ray = ComputeRay(light_origin, eye_hit);

        Vec3Df light_hit, normalized_normal;
        int i_triangle_hit;

        if (Intersect(0, light_origin, ray, light_hit, i_triangle_hit, normalized_normal)) {
            if (i_triangle_hit == triangle_index) {
                // the closest triangle is the one we are trying to shade.
                // so it's directly illuminiated by this light.
                factor += 0.8;
            } else {
                // i_triangle_hit is in between the light and the triangle we are
                // currently shading. This light doesn't illuminiate this spot.

                // nothing is added to this color.
            }
        }
    }
    factor = fmin(factor, 1);


    direct_color = Vec3Df(0, 0, 0);
    direct_color += MyMesh.materials[MyMesh.triangleMaterials[triangle_index]].Kd() * factor;
}

void Shade(const int level, const Vec3Df &ray_destination, const Vec3Df &hit, const int &i_triangle_hit, const Vec3Df &normalized_normal, Vec3Df &color) {
    float reflection, transmission;
    Vec3Df direct_color, reflected_color, refracted_color;

    Material triangle_material = MyMesh.materials[MyMesh.triangleMaterials[i_triangle_hit]];

    #pragma omp parallel sections
    {
        // Direct light
        #pragma omp section
        {
            ComputeDirectLight(hit, i_triangle_hit, direct_color);
        }

        // Reflected light
        #pragma omp section
        {
            if (triangle_material.has_Tr() && triangle_material.Tr() < 1.0f && level < maxLevel) {
                Vec3Df reflected_ray = ComputeReflectedRay(
                    ray_destination,
                    normalized_normal
                );
                Vec3Df reflection_origin = hit + reflected_ray * 10000; //TODO change 10000 to end of box
                Trace(level + 1, hit, reflected_ray, reflection_origin, reflected_color);
                reflection = 1 - triangle_material.Tr();
            }
        }

        // Refracted light
        #pragma omp section
        {
            //if material refracts && (level < maxLevel)
            //    ComputeRefractedRay(hit, &refractedRay);
            //    Trace(level+1, refractedRay, &refractedColor);
            transmission = 0; //TODO
            refracted_color = Vec3Df(0, 0, 0); //TODO
        }
    }

    color = direct_color * 0.5 + 0.5 * (reflection * reflected_color + transmission * refracted_color);
    //std::cout << direct_color << " " << reflected_color << " " << color << std::endl;
}

//@source "3D C/C++ tutorials - Ray tracing - CPU ray tracer 01 - Ray triangle intersection" http://www.3dcpptutorials.sk/index.php?id=51 2015-10-06
bool Intersect(int level, const Vec3Df &origin, const Vec3Df &ray, Vec3Df &dest, int &i_triangle_hit, Vec3Df &normalized_normal) {
    // If max level is reached
    if (level <= maxLevel && MyMesh.intersected_by(origin, dest)) {
        // Keep only the triangles which is closest
        float min_t = std::numeric_limits<float>::max();

        // For every triangle
        for (unsigned int i = 0; i < MyMesh.triangles.size(); i++) {
            Vec3Df plane_hit;
            float t; // TODO min_t < max_distance based on dest?

            // (jieter) If I use stuff like `triangle.normal here`, it is the zero vector :(
            // Triangle triangle = MyMesh.triangles[i];

            // Ray hits plane of triangle
            float rayDotNP = MyMesh.triangles[i].rayDotNP(ray);
            if (rayDotNP != 0.0f) {

                // Distance from origin to dest
                t = MyMesh.triangles[i].distance(ray, origin);

                // Point in plane
                plane_hit = origin + ray * t;

                // Hit within triangle
                bool triangle_hit = MyMesh.triangles[i].contains(plane_hit);

                #pragma omp single
                {
                    if (
                        // If in front of camera
                        t > 0 &&
                        // Triangle closer than closest triangle currently found
                        t < min_t &&
                        // And plane hit point is within triangle
                        triangle_hit
                    ) {
                        // Set result as current
                        dest = plane_hit + (rayDotNP > 0
                            ?   MyMesh.triangles[i].normal * EPSILON
                            : - MyMesh.triangles[i].normal * EPSILON);
                        // dest = plane_hit;
                        i_triangle_hit = i;
                        normalized_normal = rayDotNP > 0
                            ?   MyMesh.triangles[i].normal
                            : - MyMesh.triangles[i].normal;
                        min_t = t;
                    }
                }
            }
        }

        // If any triangle hit, P is inside the triangle
        return min_t < std::numeric_limits<float>::max();
    } else {
        return false;
    }
}

const Vec3Df ComputeRay(const Vec3Df &origin, const Vec3Df &dest) {
    return dest - origin;
}

void Trace(int level, const Vec3Df &origin, Vec3Df &ray, Vec3Df &dest, Vec3Df &color) {
    int i_triangle_hit;
    Vec3Df normalized_normal;
    if (Intersect(level, origin, ray, dest, i_triangle_hit, normalized_normal)) {
        ray = ComputeRay(origin, dest);
        Shade(level, ray, dest, i_triangle_hit, normalized_normal, color);
    } else {
        color = backgroundColor;
    }
    if (mode == MODE_INTERACTIVE_RAYTRACE) {
        std::vector<Vec3Df> test_ray;
        test_ray.push_back(Vec3Df(color));
        test_ray.push_back(origin);
        test_ray.push_back(dest);
        test_rays.push_back(test_ray);
    }
}

// return the color of your pixel.
// Function is called for every pixel in the view when pressing 'r'.
Vec3Df performRayTracing(const Vec3Df & origin, Vec3Df & dest) {
    Vec3Df color;
    Vec3Df ray = ComputeRay(origin, dest);
    Trace(0, origin, ray, dest, color);
    return color;
}


void yourDebugDraw() {
    // draw open gl debug stuff
    // this function is called every frame

    // let's draw the mesh
    MyMesh.draw();

    // let's draw the lights in the scene as points
    glPushAttrib(GL_ALL_ATTRIB_BITS); //store all GL attributes
    glDisable(GL_LIGHTING);
    glColor3f(1, 1, 1);
    glPointSize(10);
    glBegin(GL_POINTS);
    for(int i = 0; i < MyLightPositions.size(); ++i) {
        glVertex3fv(MyLightPositions[i].pointer());
    }
    glEnd();
    glPopAttrib(); // restore all GL attributes
    // The Attrib commands maintain the state.
    // e.g., even though inside the two calls, we set
    // the color to white, it will be reset to the previous
    // state after the pop.


    // as an example: we draw the test ray, which is set by the keyboard function
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_LIGHTING);
    glBegin(GL_LINES);
    std::vector<Vec3Df> test_ray;
    Vec3Df test_ray_color;
    Vec3Df test_ray_origin;
    Vec3Df test_ray_destination;
    for (int j = 0; j < test_rays.size(); ++j) {
        test_ray = test_rays[j];
        test_ray_color = test_ray[0];
        test_ray_origin = test_ray[1];
        test_ray_destination = test_ray[2];
        glColor3f(test_ray_color[0], test_ray_color[1], test_ray_color[2]);
        //glColor3f(test_ray_color.pointer());
        //glVertex3f(test_ray_origin[0], test_ray_origin[1], test_ray_origin[2]);
        glVertex3fv(test_ray_origin.pointer());
        //glVertex3f(test_ray_destination[0], test_ray_destination[1], test_ray_destination[2]);
        glVertex3fv(test_ray_destination.pointer());
    }
    glEnd();
    glPointSize(10);
    glBegin(GL_POINTS);
    glVertex3fv(MyLightPositions[0].pointer());
    glEnd();
    glPopAttrib();

    // draw whatever else you want...
    // glutSolidSphere(1,10,10);
    // allows you to draw a sphere at the origin.
    // using a glTranslate, it can be shifted to whereever you want
    // if you produce a sphere renderer, this
    // triangulated sphere is nice for the preview
}


// yourKeyboardFunc is used to deal with keyboard input.
// t is the character that was pressed
// x, y is the mouse position in pixels
// rayOrigin, rayDestination is the ray that is going in the view direction UNDERNEATH your mouse position.
//
// A few keys are already reserved:
// 'L' adds a light positioned at the camera location to the MyLightPositions vector
// 'l' modifies the last added light to the current
//     camera position (by default, there is only one light, so move it with l)
//     ATTENTION These lights do NOT affect the real-time rendering.
//     You should use them for the raytracing.
// 'r' calls the function performRaytracing on EVERY pixel, using the correct associated ray.
//     It then stores the result in an image "result.ppm".
//     Initially, this function is fast (performRaytracing simply returns
//     the target of the ray - see the code above), but once you replaced
//     this function and raytracing is in place, it might take a
//     while to complete...
void yourKeyboardFunc(char t, int x, int y, const Vec3Df & rayOrigin, const Vec3Df & rayDestination) {

    // here, as an example, I use the ray to fill in the values for my upper global ray variable
    // I use these variables in the debugDraw function to draw the corresponding ray.
    // try it: Press a key, move the camera, see the ray that was launched as a line.


    switch (t) {
    case 3: // control-c
    case 'q':
        exit(0);
        break;
    }

    std::cout << t << " pressed! The mouse was in location " << x << "," << y << "!" << std::endl;

}

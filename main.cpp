// /**
//  * Bresenham’s Line Drawing Algorithm
// */
// #include "tgaimage.h"
// #include "geometry.h"
// #include "model.h"
// #include <iostream>
// #include <vector>
// #include <cmath>
// using namespace std;

// const TGAColor white = TGAColor(255, 255, 255, 255);
// const TGAColor red   = TGAColor(255, 0,   0,   255);
// const TGAColor green = TGAColor(0,   255, 128, 255);
// Model* model = nullptr;
// const int Height = 800;
// const int Width  = 800;
// const char *fileName = "obj/african_head.obj";

// //new method: 用 barycentric画:
// // barycentric会用重心坐标判断点P是否在由pts0,pts1,pts2三个顶点组成的三角形内，如果是就返回true，否则返回false
// Vec3f barycentric(Vec3f *pts, Vec3f P){
//     Vec3f A = pts[0];
//     Vec3f B = pts[1];
//     Vec3f C = pts[2];
//     float alpha = (-((float)P.x - B.x)*(C.y - B.y) + (P.y - B.y)*(C.x - B.x)) / 
//     (-(A.x - B.x)*(C.y - B.y) + (A.y - B.y)*(C.x - B.x));

//     float beta = (-((float)P.x - C.x)*(A.y - C.y) + (P.y - C.y)*(A.x - C.x)) / 
//     (-(B.x - C.x)*(A.y - C.y) + (B.y - C.y)*(A.x - C.x));

//     float gamma = 1.0f - alpha - beta;

//     return Vec3f(alpha, beta, gamma);
// }

// void triangle(Vec3f *pts, float **zbuffer, TGAImage &image, TGAColor color){
//     //对于每个三角形，我们会在屏幕内找到最小的能够把它包裹起来的box，然后在这个box内部进行shading（提高效率）
//     Vec2f miniBound(image.get_width()-1, image.get_height()-1);
//     Vec2f maxBound(0,0);

//     //遍历三角形的三个顶点，找到每个三角形的最高点与最低点，最左点与最右点，这个三角形一定在这个box内
//     for(int i = 0; i < 3; i++){
//         miniBound.x = max(0.f, min(miniBound.x, pts[i].x));
//         miniBound.y = max(0.f, min(miniBound.y, pts[i].y));

//         maxBound.x = min((float)image.get_width()-1, max(maxBound.x, pts[i].x));
//         maxBound.y = min((float)image.get_height()-1, max(maxBound.y, pts[i].y));
//     }

//     //定义点P。P可以是box内的任意一个点，我们需要逐个遍历，如果P刚好在三角形内部，那么我们就把这个三角形着色shading
//     Vec3f P;
//     for(int i = miniBound.x; i <= maxBound.x; i++){
//         for(int j = miniBound.y; j <= maxBound.y; j++){
//             P = Vec3f(i, j, 0);    //取得当前点P
//             Vec3f baryCentricCoord = barycentric(pts, P);
//             //求出重心坐标alpha, beta, lambda。当且仅当这三个值都大于0时，点才会在三角形平面内
//             if(baryCentricCoord.x >= 0.0f && baryCentricCoord.y >= 0.f && baryCentricCoord.z >= 0.f){  //如果当前点P在三角形内，就shading
//                 for(int k = 0; k < 3; k++)  P.z += pts[k].z * baryCentricCoord[k];
//                 if (zbuffer[(int)P.x][(int)P.y]<P.z) {
//                     zbuffer[(int)P.x][(int)P.y] = P.z;
//                     image.set(P.x, P.y, color);
//                 }
//             }
//         }
//     }
// }

// int main(int argc, char** argv){
//     //flat shader + simple shadow
//     TGAImage image(Width, Height, TGAImage::RGB);
//     if(argc == 2)
//         model = new Model(argv[1]);
//     else
//         model = new Model(fileName);
//     Vec3f light_dir(0., 0., -1.);   //定义光源的位置（用于shadowing）
//     float **zbuffer = new float*[Width];
//     for(int i = 0; i < Width; i++){
//         zbuffer[i] = new float[Height];
//         for(int j = 0; j < Height; j++){
//             zbuffer[i][j] = -std::numeric_limits<float>::max();
//         }
//     }

//     //遍历所有的面，每个face都是一个三角形，有三个顶点vertex，每个vertex有(x,y,z)三个坐标
//     for(int i = 0; i < model->nfaces(); i++){
//         vector<int> face = model->face(i); //用vector取出每个face，方便后续操作
//         Vec3f screenCoord[3];   //保存三角形三个vertex在屏幕上出现的位置
//         Vec3f worldCoord[3];    //保存三角形三个vertex的实际坐标
//         //遍历三个顶点
//         for(int j = 0; j < 3; j++){
//             worldCoord[j] = model->vert(face[j]);   //用vert()取出该vertex的坐标(x,y)并保存在worldCoord中
//             screenCoord[j] = Vec3f(int((worldCoord[j].x + 1) * Width/2. + .5), int((worldCoord[j].y + 1) * Height/2. + .5), worldCoord[j].z);    //转换到屏幕上的位置
//         }
//         // 求出三角形平面的法线。
//         Vec3f n = cross(Vec3f(worldCoord[2] - worldCoord[0]), Vec3f(worldCoord[1] - worldCoord[0]));
//         n = n.normalize();
//         // 穿过三角形平面的光的intensity与光源和平面法线的夹脚有关，当光来源与平面平行时，基本没有光穿过平面，可以不着色（阴影）。因此我们此处用点乘
//         float intensity = n * light_dir;
//         if(intensity > 0)   //只有有光穿过平面时我们才会着色，且颜色深度与intensity有关。
//             triangle(screenCoord, zbuffer, image, TGAColor(intensity*255, intensity*255, intensity*255, 255));
//     }

//     image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
//     image.write_tga_file("outputZBuffer.tga");
//     delete model;
//     delete []zbuffer;
//     return 0;
// }

#include <vector>
#include <cmath>
#include <limits>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const int width  = 800;
const int height = 800;
const int depth  = 255;

Model *model = NULL;
int *zbuffer = NULL;
Vec3f light_dir(0,0,-1);
Vec3f camera(0,0,3);

Vec3f m2v(Matrix m) {
    return Vec3f(m[0][0]/m[3][0], m[1][0]/m[3][0], m[2][0]/m[3][0]);
}

Matrix v2m(Vec3f v) {
    Matrix m(4, 1);
    m[0][0] = v.x;
    m[1][0] = v.y;
    m[2][0] = v.z;
    m[3][0] = 1.f;
    return m;
}

Matrix viewport(int x, int y, int w, int h) {
    Matrix m = Matrix::identity(4);
    m[0][3] = x+w/2.f;
    m[1][3] = y+h/2.f;
    m[2][3] = depth/2.f;

    m[0][0] = w/2.f;
    m[1][1] = h/2.f;
    m[2][2] = depth/2.f;
    return m;
}

void triangle(Vec3i t0, Vec3i t1, Vec3i t2, Vec2i uv0, Vec2i uv1, Vec2i uv2, TGAImage &image, float intensity, int *zbuffer) {
    if (t0.y==t1.y && t0.y==t2.y) return; // i dont care about degenerate triangles
    if (t0.y>t1.y) { std::swap(t0, t1); std::swap(uv0, uv1); }
    if (t0.y>t2.y) { std::swap(t0, t2); std::swap(uv0, uv2); }
    if (t1.y>t2.y) { std::swap(t1, t2); std::swap(uv1, uv2); }

    int total_height = t2.y-t0.y;
    for (int i=0; i<total_height; i++) {
        bool second_half = i>t1.y-t0.y || t1.y==t0.y;
        int segment_height = second_half ? t2.y-t1.y : t1.y-t0.y;
        float alpha = (float)i/total_height;
        float beta  = (float)(i-(second_half ? t1.y-t0.y : 0))/segment_height; // be careful: with above conditions no division by zero here
        Vec3i A   =               t0  + Vec3f(t2-t0  )*alpha;
        Vec3i B   = second_half ? t1  + Vec3f(t2-t1  )*beta : t0  + Vec3f(t1-t0  )*beta;
        Vec2i uvA =               uv0 +      (uv2-uv0)*alpha;
        Vec2i uvB = second_half ? uv1 +      (uv2-uv1)*beta : uv0 +      (uv1-uv0)*beta;
        if (A.x>B.x) { std::swap(A, B); std::swap(uvA, uvB); }
        for (int j=A.x; j<=B.x; j++) {
            float phi = B.x==A.x ? 1. : (float)(j-A.x)/(float)(B.x-A.x);
            Vec3i   P = Vec3f(A) + Vec3f(B-A)*phi;
            Vec2i uvP =     uvA +   (uvB-uvA)*phi;
            int idx = P.x+P.y*width;
            if (zbuffer[idx]<P.z) {
                zbuffer[idx] = P.z;
                TGAColor color = model->diffuse(uvP);
                image.set(P.x, P.y, TGAColor(color.r*intensity, color.g*intensity, color.b*intensity));
            }
        }
    }
}

int main(int argc, char** argv) {
    if (2==argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("obj/african_head.obj");
    }

    zbuffer = new int[width*height];
    for (int i=0; i<width*height; i++) {
        zbuffer[i] = std::numeric_limits<int>::min();
    }

    { // draw the model
        Matrix Projection = Matrix::identity(4);
        Matrix ViewPort   = viewport(width/8, height/8, width*3/4, height*3/4);
        Projection[3][2] = -1.f/camera.z;

        TGAImage image(width, height, TGAImage::RGB);
        for (int i=0; i<model->nfaces(); i++) {
            std::vector<int> face = model->face(i);
            Vec3i screen_coords[3];
            Vec3f world_coords[3];
            for (int j=0; j<3; j++) {
                Vec3f v = model->vert(face[j]);
                screen_coords[j] =  m2v(ViewPort*Projection*v2m(v));
                world_coords[j]  = v;
            }
            Vec3f n = (world_coords[2]-world_coords[0])^(world_coords[1]-world_coords[0]);
            n.normalize();
            float intensity = n*light_dir;
            if (intensity>0) {
                Vec2i uv[3];
                for (int k=0; k<3; k++) {
                    uv[k] = model->uv(i, k);
                }
                triangle(screen_coords[0], screen_coords[1], screen_coords[2], uv[0], uv[1], uv[2], image, intensity, zbuffer);
            }
        }

        image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
        image.write_tga_file("outputTexture.tga");
    }

    { // dump z-buffer (debugging purposes only)
        TGAImage zbimage(width, height, TGAImage::GRAYSCALE);
        for (int i=0; i<width; i++) {
            for (int j=0; j<height; j++) {
                zbimage.set(i, j, TGAColor(zbuffer[i+j*width], 1));
            }
        }
        zbimage.flip_vertically(); // i want to have the origin at the left bottom corner of the image
        zbimage.write_tga_file("zbuffer.tga");
    }
    delete model;
    delete [] zbuffer;
    return 0;
}
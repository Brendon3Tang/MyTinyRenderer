/**
 * Bresenham’s Line Drawing Algorithm
*/
#include "tgaimage.h"
#include "geometry.h"
#include "model.h"
#include <iostream>
#include <vector>
#include <cmath>
using namespace std;

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
const TGAColor green = TGAColor(0,   255, 128, 255);
Model* model = nullptr;
const int Height = 800;
const int Width  = 800;
const char *fileName = "obj/african_head.obj";

/*
// Old-school method: Line sweeping：
void triangle(Vec2i t0, Vec2i t1, Vec2i t2, TGAImage &image, TGAColor color) { 
    if (t0.y==t1.y && t0.y==t2.y) return;
    // sort the vertices, t0, t1, t2 lower−to−upper (bubblesort yay!) 
    if (t0.y>t1.y) std::swap(t0, t1); 
    if (t0.y>t2.y) std::swap(t0, t2); 
    if (t1.y>t2.y) std::swap(t1, t2); 
    int total_height = t2.y-t0.y; 
    // for (int y=t0.y; y<=t1.y; y++) { 
    //     int segment_height = t1.y-t0.y+1; 
    //     float alpha = (float)(y-t0.y)/total_height; 
    //     float beta  = (float)(y-t0.y)/segment_height; // be careful with divisions by zero 
    //     Vec2i A = t0 + (t2-t0)*alpha; 
    //     Vec2i B = t0 + (t1-t0)*beta; 
    //     if (A.x>B.x) std::swap(A, B); 
    //     for (int j=A.x; j<=B.x; j++) { 
    //         image.set(j, y, color); // attention, due to int casts t0.y+i != A.y 
    //     } 
    // } 
    // for (int y=t1.y; y<=t2.y; y++) { 
    //     int segment_height =  t2.y-t1.y+1; 
    //     float alpha = (float)(y-t0.y)/total_height; 
    //     float beta  = (float)(y-t1.y)/segment_height; // be careful with divisions by zero 
    //     Vec2i A = t0 + (t2-t0)*alpha; 
    //     Vec2i B = t1 + (t2-t1)*beta; 
    //     if (A.x>B.x) std::swap(A, B); 
    //     for (int j=A.x; j<=B.x; j++) { 
    //         image.set(j, y, color); // attention, due to int casts t0.y+i != A.y 
    //     } 
    // }

    //整合一下，用一个for loop一起画
    for(int y = t0.y; y < t2.y; y++){
        bool second_half = y > t1.y || (t1.y == t0.y);
        int segment_height = second_half ? t2.y-t1.y : t1.y-t0.y;
        float alpha = (float)(y - t0.y)/total_height;
        float beta  = (float)(y-(second_half ? t1.y : t0.y))/segment_height;
        Vec2i A = t0 + (t2 - t0) * alpha;
        Vec2i B = second_half ? t1 + (t2 - t1) * beta : t0 + (t1 - t0) * beta;

        if(A.x > B.x)   swap(A, B);
        for(int j = A.x; j <= B.x; j++){
            image.set(j, y, color);
        }
    } 
}
*/

//new method: 用 barycentric画:
// barycentric会用重心坐标判断点P是否在由pts0,pts1,pts2三个顶点组成的三角形内，如果是就返回true，否则返回false
bool barycentric(Vec2i *pts, Vec2i P){
    Vec2i A = pts[0];
    Vec2i B = pts[1];
    Vec2i C = pts[2];
    float alpha = (-((float)P.x - B.x)*(C.y - B.y) + (P.y - B.y)*(C.x - B.x)) / 
    (-(A.x - B.x)*(C.y - B.y) + (A.y - B.y)*(C.x - B.x));

    float beta = (-((float)P.x - C.x)*(A.y - C.y) + (P.y - C.y)*(A.x - C.x)) / 
    (-(B.x - C.x)*(A.y - C.y) + (B.y - C.y)*(A.x - C.x));

    float lambda = 1.0f - alpha - beta;
    //求出重心坐标alpha, beta, lambda。当且仅当这三个值都大于0时，点才会在三角形平面内
    if(alpha >= 0.0f && beta >= 0.f && lambda >= 0.f) 
        return true;

    return false;
}

void triangle(Vec2i *pts, TGAImage &image, TGAColor color){
    //对于每个三角形，我们会在屏幕内找到最小的能够把它包裹起来的box，然后在这个box内部进行shading（提高效率）
    Vec2i miniBound(image.get_width()-1, image.get_height()-1);
    Vec2i maxBound(0,0);

    //遍历三角形的三个顶点，找到每个三角形的最高点与最低点，最左点与最右点，这个三角形一定在这个box内
    for(int i = 0; i < 3; i++){
        miniBound.x = max(0, min(miniBound.x, pts[i].x));
        miniBound.y = max(0, min(miniBound.y, pts[i].y));

        maxBound.x = min(image.get_width()-1, max(maxBound.x, pts[i].x));
        maxBound.y = min(image.get_height()-1, max(maxBound.y, pts[i].y));
    }

    //定义点P。P可以是box内的任意一个点，我们需要逐个遍历，如果P刚好在三角形内部，那么我们就把这个三角形着色shading
    Vec2i P;
    for(int i = miniBound.x; i <= maxBound.x; i++){
        for(int j = miniBound.y; j <= maxBound.y; j++){
            P = Vec2i(i, j);    //取得当前点P
            if(barycentric(pts, P)) //如果当前点P在三角形内，就shading
                image.set(P.x, P.y, color);
        }
    }
}

int main(int argc, char** argv){
    //lesson 2
    // old way画三角形
    //定义三个三角形
    // Vec2i t0[3] = {Vec2i(10, 70),   Vec2i(50, 160),  Vec2i(70, 80)}; 
    // Vec2i t1[3] = {Vec2i(180, 50),  Vec2i(150, 1),   Vec2i(70, 180)}; 
    // Vec2i t2[3] = {Vec2i(180, 150), Vec2i(120, 160), Vec2i(130, 180)}; 
    //画出三个三角形
    // triangle(t0[0], t0[1], t0[2], image, red); 
    // triangle(t1[0], t1[1], t1[2], image, white); 
    // triangle(t2[0], t2[1], t2[2], image, green);
    
    //barycentric画三角形
    // TGAImage image(200, 200, TGAImage::RGB);
    // Vec2i pts[3] = {Vec2i(10,10), Vec2i(100, 30), Vec2i(190, 160)}; //画出这个三角形
    // triangle(pts, image, TGAColor(255, 0, 0, 255));

    //flat shader + simple shadow
    TGAImage image(Width, Height, TGAImage::RGB);
    if(argc == 2)
        model = new Model(argv[1]);
    else
        model = new Model(fileName);
    Vec3f light_dir(0., 0., -1.);   //定义光源的位置（用于shadowing）

    //遍历所有的面，每个face都是一个三角形，有三个顶点vertex，每个vertex有(x,y,z)三个坐标
    for(int i = 0; i < model->nfaces(); i++){
        vector<int> face = model->face(i); //用vector取出每个face，方便后续操作
        Vec2i screenCoord[3];   //保存三角形三个vertex在屏幕上出现的位置
        Vec3f worldCoord[3];    //保存三角形三个vertex的实际坐标
        //遍历三个顶点
        for(int j = 0; j < 3; j++){
            worldCoord[j] = model->vert(face[j]);   //用vert()取出该vertex的坐标(x,y)并保存在worldCoord中
            screenCoord[j] = Vec2i((worldCoord[j].x + 1) * Width/2., (worldCoord[j].y + 1) * Height/2.);    //转换到屏幕上的位置
        }

        // 求出三角形平面的法线。
        Vec3f n = Vec3f(worldCoord[2] - worldCoord[0]) ^ Vec3f(worldCoord[1] - worldCoord[0]);
        n = n.normalize();
        // 穿过三角形平面的光的intensity与光源和平面法线的夹脚有关，当光来源与平面平行时，基本没有光穿过平面，可以不着色（阴影）。因此我们此处用点乘
        float intensity = n * light_dir;
        if(intensity > 0)   //只有有光穿过平面时我们才会着色，且颜色深度与intensity有关。
            triangle(screenCoord, image, TGAColor(intensity*255, intensity*255, intensity*255, 255));
    }

    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("outputFlatShaderShadow.tga");
    delete model;
    return 0;
}

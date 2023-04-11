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


// Old-school method: Line sweeping：假设三角形有A,B,C三个顶点，我们按y坐标从大到小排序。
//假设排序后我们按从高到低顺序有顶点t2、t1、t0。然后y从t0开始不断往上朝着t1与t2移动，画出三角形的两条边，然后对于y相同的点，我们把两条边连起来（shading）
void triangle(Vec2i t0, Vec2i t1, Vec2i t2, TGAImage &image, TGAColor color) { 
    if (t0.y==t1.y && t0.y==t2.y) return;
    // sort the vertices, t0, t1, t2 lower−to−upper (bubblesort yay!) 
    if (t0.y>t1.y) std::swap(t0, t1); 
    if (t0.y>t2.y) std::swap(t0, t2); 
    if (t1.y>t2.y) std::swap(t1, t2); 
    int total_height = t2.y-t0.y; 
    // 分开画三角形，以t1为分界线，先画三角形的下半部分，在画三角形的上半部分
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

int main(int argc, char** argv){
    TGAImage image(200, 200, TGAImage::RGB);
    //lesson 2
    // old way画三角形
    //定义三个三角形
    Vec2i t0[3] = {Vec2i(10, 70),   Vec2i(50, 160),  Vec2i(70, 80)}; 
    Vec2i t1[3] = {Vec2i(180, 50),  Vec2i(150, 1),   Vec2i(70, 180)}; 
    Vec2i t2[3] = {Vec2i(180, 150), Vec2i(120, 160), Vec2i(130, 180)}; 
    //画出三个三角形
    triangle(t0[0], t0[1], t0[2], image, red); 
    triangle(t1[0], t1[1], t1[2], image, white); 
    triangle(t2[0], t2[1], t2[2], image, green);

    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("outputFlatShaderShadow.tga");
    delete model;
    return 0;
}

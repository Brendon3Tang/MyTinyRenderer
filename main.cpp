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
// 尝试二，
//  问题一：当斜线比较陡时，由于x变化小，y变化大，因此得到的t数量较少，因此画出来的点数量少，中间空隙大
//  问题二：当x0 > x1时，即我们想从右到左画时，由于x > x1，因此for循环不会启动，我们也不会在这个方向画line
// void line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color) { 
//     for (int x=x0; x<=x1; x++) { 
//         float t = (x-x0)/(float)(x1-x0); //根据x的移动在[x0,x1]中的占比来求t
//         int y = y0*(1.-t) + y1*t; //把t带入求在此 x 时的 y 是多少
//         image.set(x, y, color); //设置x与y。
//     } 
// }

// 尝试三：
//  解决问题一的思路：当斜线比较陡峭时，我们知道dy > dx，因此我们就用y来计算t。具体实现
//方法是swap x 与 y，然后设置bool steep为true，在画图时之需要set(y,x)，而不是set(x,y)即可
//  解决问题二的思路：当从右到左时，也就是x0 > x1时，我们只需要swap(x0, x1),swap(y0,y1)即可
//因为反过来画得到的效果是一样的
//  问题：效率还是不够高，要提高速度
// void line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color) { 
//     bool steep = false; 
//     if (std::abs(x0-x1)<std::abs(y0-y1)) { // if the line is steep, we transpose the image，当斜线比较陡峭时，我们知道dy > dx，因此我们就用y来计算t
//         std::swap(x0, y0); 
//         std::swap(x1, y1); 
//         steep = true; 
//     } 
//     if (x0>x1) { // make it left−to−right，当从右到左时，也就是x0 > x1时，我们只需要swap(x0, x1),swap(y0,y1)即可，因为反过来画得到的效果是一样的
//         std::swap(x0, x1); 
//         std::swap(y0, y1); 
//     } 
//     for (int x = x0; x <= x1; x++) { 
//         float t = (x-x0)/(float)(x1-x0); 
//         int y = y0*(1.-t) + y1*t; 
//         if (steep) { 
//             image.set(y, x, color); // if transposed, de−transpose 
//         } else { 
//             image.set(x, y, color); 
//         } 
//     } 
// }

// 尝试四：
//  derror: slop
//  error: 累加器。用error来控制y的大小，而不是用t，因此省略了求t的过程，提高效率
void line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color) { 
    bool steep = false; 
    if (std::abs(x0-x1)<std::abs(y0-y1)) { // if the line is steep, we transpose the image，当斜线比较陡峭时，我们知道dy > dx，因此我们就用y来计算t
        std::swap(x0, y0); 
        std::swap(x1, y1); 
        steep = true; 
    } 
    if (x0>x1) { // make it left−to−right，当从右到左时，也就是x0 > x1时，我们只需要swap(x0, x1),swap(y0,y1)即可，因为反过来画得到的效果是一样的
        std::swap(x0, x1); 
        std::swap(y0, y1); 
    }
    int dx = x1 - x0;
    int dy = y1 - y0; 
    float derror = std::abs(dy/float(dx));//derror就是slop
    float error = 0;
    int y = y0;
    for (int x = x0; x <= x1; x++) { 
        if (steep) { 
            image.set(y, x, color); // if transposed, de−transpose 
        } else { 
            image.set(x, y, color); 
        } 

        error += derror;//x上每前进1，y就会前进derror（slop的定义），我们把它累加到error中
        if(error > .5){ //每当error累加到0.5或以上时，使y前进一格（我们不等error完全到1再加），然后使error回到-0.5左右，再继续累加
        //我们这么做是为了使pixel的坐标位置坐落于pixel的中间，而非右下角（？不太理解）
            y += (y0 > y1 ? -1 : 1);
            error -= 1.;
        }
    } 
}

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
bool barycentric(Vec2i *pts, Vec2i P){
    Vec2i A = pts[0];
    Vec2i B = pts[1];
    Vec2i C = pts[2];
    float alpha = (-((float)P.x - B.x)*(C.y - B.y) + (P.y - B.y)*(C.x - B.x)) / 
    (-(A.x - B.x)*(C.y - B.y) + (A.y - B.y)*(C.x - B.x));

    float beta = (-((float)P.x - C.x)*(A.y - C.y) + (P.y - C.y)*(A.x - C.x)) / 
    (-(B.x - C.x)*(A.y - C.y) + (B.y - C.y)*(A.x - C.x));

    float lambda = 1.0f - alpha - beta;

    if(alpha >= 0.0f && beta >= 0.f && lambda >= 0.f) 
        return true;

    return false;
}

void triangle(Vec2i *pts, TGAImage &image, TGAColor color){
    //barycentric(pts);
    Vec2i miniBound(image.get_width()-1, image.get_height()-1);
    Vec2i maxBound(0,0);

    for(int i = 0; i < 3; i++){
        miniBound.x = max(0, min(miniBound.x, pts[i].x));
        miniBound.y = max(0, min(miniBound.y, pts[i].y));

        maxBound.x = min(image.get_width()-1, max(maxBound.x, pts[i].x));
        maxBound.y = min(image.get_height()-1, max(maxBound.y, pts[i].y));
    }

    Vec2i P;
    for(int i = miniBound.x; i <= maxBound.x; i++){
        for(int j = miniBound.y; j <= maxBound.y; j++){
            P = Vec2i(i, j);
            // Vec3f baryC = barycentric(pts, P);
            // if(baryC.x < 0 || baryC.y < 0 || baryC.z < 0) continue;
            if(barycentric(pts, P))
                image.set(P.x, P.y, color);
        }
    }
}

int main(int argc, char** argv){
    // TGAImage image(Width, Height, TGAImage::RGB);
    //lesson 0
    // line(13, 20, 80, 40, image, white); 
    // line(10, 10, 70, 70, image, white); 
    // line(20, 13, 40, 80, image, red); 
    // line(80, 40, 13, 20, image, red);

    //lesson 1
    //载入object，如果command line有指定object则载入指定object，否则载入african_head
    // if(argc == 2)
    //     model = new Model(argv[1]);
    // else
    //     model = new Model(fileName);
    
    // for(int i = 0; i < model->nfaces(); i++){
    //     vector<int> face = model->face(i);//取得object的其中一个face。每个face由三个顶点组成。
    //     for(int j = 0; j < 3; j++){//取得三个顶点中的两个，并且把他们连起来
    //     //三次loop中分别找到当前的三个边(vertex0，vertex1)，(vertex1，vertex2)，(vertex2，vertex0)，然后把线画出来
    //         Vec3f v0 = model->vert(face[j]);    
    //         Vec3f v1 = model->vert(face[(j+1)%3]);
    //         //把模型点移动到image的中心：
    //         //  1. 定点坐标全部+1是为了把模型的中心的坐标从(0,0)移动到(1，1)。不移动的话模型的中心会在左下角，我们只能看到四分之一
    //         //  2. 为了让模型的大小适应image所以是把x，y坐标全部乘Width/2与Height/2。不乘的话模型会很小，只在一个pixel里显现
    //         int x0 = (v0.x + 1) * Width/2;    
    //         int y0 = (v0.y + 1) * Height/2;
    //         int x1 = (v1.x + 1) * Width/2;    
    //         int y1 = (v1.y + 1) * Height/2;
    //         line(x0, y0, x1, y1, image, green);
    //     }
    // }

    //lesson 2
    // old way画三角形
    // Vec2i t0[3] = {Vec2i(10, 70),   Vec2i(50, 160),  Vec2i(70, 80)}; 
    // Vec2i t1[3] = {Vec2i(180, 50),  Vec2i(150, 1),   Vec2i(70, 180)}; 
    // Vec2i t2[3] = {Vec2i(180, 150), Vec2i(120, 160), Vec2i(130, 180)}; 
    // triangle(t0[0], t0[1], t0[2], image, red); 
    // triangle(t1[0], t1[1], t1[2], image, white); 
    // triangle(t2[0], t2[1], t2[2], image, green);
    
    //barycentric画三角形
    // TGAImage image(200, 200, TGAImage::RGB);
    // Vec2i pts[3] = {Vec2i(10,10), Vec2i(100, 30), Vec2i(190, 160)}; 
    // triangle(pts, image, TGAColor(255, 0, 0, 255));

    TGAImage image(Width, Height, TGAImage::RGB);
    if(argc == 2)
        model = new Model(argv[1]);
    else
        model = new Model(fileName);
    for(int i = 0; i < model->nfaces(); i++){
        vector<int> face = model->face(i); 
        Vec2i screenCoord[3];
        for(int j = 0; j < 3; j++){
            Vec3f worldCoord = model->vert(face[j]);
            screenCoord[j] = Vec2i((worldCoord.x + 1) * Width/2., (worldCoord.y + 1) * Height/2.);
        }
        triangle(screenCoord, image, TGAColor(rand()%255, rand()%255, rand()%255, 255));
    }

    // flat shader

    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("outputFlatShader.tga");
    delete model;
    return 0;
}

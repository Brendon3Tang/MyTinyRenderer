# TinyRenderer 项目学习笔记：
## Lesson 1
1. 使用重心坐标公式得不出需要的结果：没有注意int与float直接的互换。输入的三角形点坐标是int，但是重心坐标是float。

## Lesson 2

## Lesson 3 Z-Buffer & Texture Mapping
### 课程总结：
1. 为三角形的光栅化增加深度测试，做出阴影，具体步骤，在triangle函数中：
   1. 创建zBuffer缓存，用于存储物体的z值，z值越大，表示物体离camera的距离越近。
   2. 如果在当前pixel的坐标，点P的坐标，他的z值大于zBuffer中已有的值，说明他离camera更近，我们为这个物体在这个pixel里shading，并更新zBuffer的值
2. 使用纹理映射为模型贴图，具体步骤：
   1. 从模型文件obj中加载纹理坐标(u,v)并存入vector2i uv_中
   2. 调用model中的函数load_texture，把texture加载到一个TGAImage中作为贴图
   3. 根据obj中的索引(f的信息)，得到当前face的当前vertex的纹理坐标的索引值(faces[nface][nvertex][1])，然后根据索引找到纹理坐标，完成了纹理映射
   4. 使用坐标(u,v)取得纹理贴图中该点的颜色数据，并作为color给三角形上色
3. model文件中新增了：
   1. 变量：
      1. std::vector<Vec2f> uv_; 
      2. TGAImage diffusemap_; 
   2. 函数：
      1. void load_texture(std::string filename, const char *suffix, TGAImage &img)
      2. Vec2i uv(int iface, int nvert);
      3. TGAColor diffuse(Vec2i uv);
      4. std::vector<int> face(int idx);
   
### 问题总结
1. 深度测试，Z-buffer中的值该怎么求？
   - P.z的值就是三角形三个顶点ABC的z值的加权平均，即P.z = A.z * Alpha + B.z * Beta + C.z * Gamma
2. 深度测试在哪里做？
   - 在三角形的光栅化时做，即函数triangle里
3. 如何动态地创建二维数组：
```
    float **zbuffer = new float*[Width];    //创建一个指向指针数组的指针 **zbuffer
    for(int i = 0; i < Width; i++){
        zbuffer[i] = new float[Height]; //指针数组中的每个指针指向一个数组
        for(int j = 0; j < Height; j++){
            zbuffer[i][j] = -std::numeric_limits<float>::max();
        }
    }
```
4. obj文件的信息：
   - v:顶点坐标，每个顶点有（x,y,z）三个值
   - vt：纹理坐标，每个坐标有(u,v)两个值
   - vn：顶点的法线向量(x,y,z)三个值
   - f：表示一个面，每行由三个tuple组成，每个tuple是： (v的索引/vt的索引/vn的索引)
5. 如何插值：
   - zBuffer: 当我们知道三角形（object）三个顶点的z坐标（距离camera的距离）后，就能通过插值的方式知道点P对应的z坐标（距离camera的距离）：
     - 通过obj file得到三角形三个顶点坐标pts[0], pts[1], pts[2]
     - 通过P点的重心坐标(alpha, beta, gamma)，我们得到P点对应的z坐标：
       - P.z = pts[0].z * alpha + pts[1].z * beta + pts[2].z * gamma
   - 纹理映射：当我们知道三角形三个顶点对应的纹理坐标后，就能通过插值的方式知道点P对应的纹理坐标：
     - 通过obj file得到三角形顶点的纹理坐标uv[0], uv[1], uv[2]
     - 通过P点的重心坐标(alpha, beta, gamma)，我们得到P点对应的纹理坐标uvP:
       - uvP.u = uv[0].u * alpha + uv[1].u * beta + uv[2].u * gamma
       - uvP.v = uv[0].v * alpha + uv[1].v * beta + uv[2].v * gamma
```
for(int k = 0; k < 3; k++){
  P.z += pts[k].z * baryCentricCoord[k];   //P.z的插值就是三角形三个顶点ABC的z值的加权平均，即A.z * Alpha(顶点A) + B.z * Beta（顶点B） + C.z * Gamma(顶点C)
  uvP.x += uv[k].x * baryCentricCoord[k]; //同理，插值后P的纹理坐标uvP.x的值就是三角形三个顶点对应的纹理坐标的值的加权平均，
  uvP.y += uv[k].y * baryCentricCoord[k]; // 插值后P的纹理坐标y同理
}
```
6. 如何读取obj文件？
   - 参考model.cpp里的文件读取函数的注释

## Lesson 4 Perspective Projection
### 课程要点：
1. We know that diagonal coefficients of the matrix scale our world along the coordinate axes
2. Homogeneous coordinates方便了人们用单个矩阵同时表示物体的scaling，rotating，还有translating
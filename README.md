# TinyRenderer 项目学习笔记(Open in a Markdown reader)：
## Lesson 1
1. 使用重心坐标公式得不出需要的结果：没有注意int与float直接的互换。输入的三角形点坐标是int，但是重心坐标是float。

## Lesson 2
### 课程总结：
1. 插值，barycentric
### 问题总结：
1. 改写了他的barycentric，使用新的

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
### 主要内容：
1. We know that diagonal coefficients of the matrix scale our world along the coordinate axes
2. Homogeneous coordinates方便了人们用单个矩阵同时表示物体的scaling，rotating，还有translating
3. In homogeneous coordinates all things with z=0 are vectors, all the rest are points。在齐次坐标系里，z = 0的是向量，其他的是点。
4. 方法一：求Perspective Projection的matrix（**与大部分的定义不同，摄像机的位置在(0,0,c)**）：
   1. 先看一个矩阵乘法，用齐次坐标系，我们用矩阵 M 把任意一个点 K: $(x, y, z)$变换到点 K': $(\frac{x}{rz+1}, \frac{y}{rz+1},\frac{z}{rz+1}) $: 
   \
   $\begin{bmatrix}
      1 & 0 & 0 & 0\\
      0 & 1 & 0 & 0\\
      0 & 0 & 1 & 0\\
      0 & 0 & r & 1\\
   \end{bmatrix} * 
   \begin{bmatrix}
      x\\
      y\\
      z\\
      1\\
   \end{bmatrix} = 
   \begin{bmatrix}
      x\\
      y\\
      z\\
      rz + 1\\
   \end{bmatrix}$
   \
   经过变换后：得到结果点K'
   \
   $\begin{bmatrix}
      x\\
      y\\
      z\\
      rz + 1\\
   \end{bmatrix} = 
   \begin{bmatrix}
      \frac{x}{rz+1}\\
      \frac{y}{rz+1}\\
      \frac{z}{rz+1}
   \end{bmatrix}$  
   <br/>
   2. 然后看下图中的三角形：
    ![image from the repo lesson 5](LESSON/img/PerspectiveProjection.png)
      1. $\bigtriangleup ABC$与$\bigtriangleup ODC$是相似三角形:
         - $(\frac{|AB|}{|AC|} = \frac{|OD|}{|OC|}) \Rightarrow (\frac{x}{c-z} = \frac{x'}{c}) \Rightarrow (x' = \frac{x}{1- \frac{z}{c}})$
      2. 同理可得：$ y' = \frac{y}{1 - \frac{z}{c}}$
      3. 最终结果得到一个点P'为 $
         \begin{bmatrix}
         \frac{x}{1- \frac{z}{c}}\\
         \frac{y}{1 - \frac{z}{c}}\\
         \frac{z}{1 - \frac{z}{c}} = 0
      \end{bmatrix}$。
      当$r = -\frac{1}{c}$时，K' = $ \begin{bmatrix}
         \frac{x}{1- \frac{z}{c}}\\
         \frac{y}{1 - \frac{z}{c}}\\
         \frac{z}{1 - \frac{z}{c}} 
      \end{bmatrix}$。由此得出，当我们把 $r = -\frac{1}{c}$ 带入矩阵M，可以得到Perspective Projection 矩阵：$\begin{bmatrix}
      1 & 0 & 0 & 0\\
      0 & 1 & 0 & 0\\
      0 & 0 & 1 & 0\\
      0 & 0 & -\frac{1}{c} & 1\\
   \end{bmatrix}$。该矩阵可以把任意点(x,y,z)投射到平面z = 0上。我们只需要用x', y'就可以得到物品在平面上的位置，z'可以用了完成z-buffer。
   4. 此种方法由于camera在(0,0,c)上，因此在求ModelView矩阵时，我们要使用的矩阵是：$\begin{bmatrix}
      x[0] & x[1] & x[2] & 0\\
      y[0] & y[1] & y[2] & 0\\
      z[0] & z[1] & z[2] & 0\\
      -target[0] & -target[1] & -target[2] & 1\\
   \end{bmatrix}$
5. 方法二：求Perspective Projection的matrix。（**大部分的定义方式，摄像机的位置在原点(0,0,0)**）：
   1. 使用以下投影矩阵([推导过程在这里](https://zhuanlan.zhihu.com/p/104039832))：
   ![formula](LESSON/img/PerspectiveProjectionFormula.png)
   [投影矩阵函数来源戳这里<----------](https://stackoverflow.com/questions/18404890/how-to-build-perspective-projection-matrix-no-api)
   2. 根据[Learn WebGL](http://learnwebgl.brown37.net/08_projections/projections_perspective.html)可知：
      1. aspect是Width/Height。
      2. fovy一般取区间[30,60]度，fovy在放入tan()函数中计算时要变成rad，假设degree = 30度：fovy_rad = 30.f * M_PI/180.f。
      3. fovy越大，物品看起来越远，fovy越小，物品看起来越大
      4. 经典的near和far取值范围是[0.1, 100]。
      5. 示例中，aspect = 1， fovy = 45度， near = 10， far = 20
   3.  此种方法由于camera在(0,0,0)上，因此在求ModelView矩阵时，我们要使用的矩阵是**M的逆矩阵**：$M = \begin{bmatrix}
      x[0] & y[0] & z[0] & eye[0]\\
      x[1] & y[1] & z[1] & eye[1]\\
      x[2] & y[2] & z[2] & eye[2]\\
      0 & 0 & 0 & 1\\
   \end{bmatrix}$,我们返回$M.inverse()$
### 难题：
1. 怎么求出$r = -\frac{1}{c}$ 
2. 怎么确定fovy（$\theta$），aspect，near，far的值


## Lesson 5 Moving the camera
### 主要内容：
1. Gouraud shading：
   1. Gouraud shading中的点P对应的纹理坐标(u,v)的intensity是根据三个vertex的intensity的插值得到的。vertex的intensity = vertex normal vector * light_dir.normal
   2. flat shading中整个三角形的intensity = 三角形的normal vector * light_dir.normal
2. 计算ModelView变换矩阵
3. 计算ViewPort变换矩阵


## Lesson 6: Shaders for the software renderer
### 主要内容：
1.  学习Normal Mapping：
    1. **normal texture法线贴图是五颜六色的，因为每个位置点P(u,v)的RGB值(r, g, b)用来表示这个点P的normal的(x, y, z)。**
    2. Normal Mapping不再需要为intensity插值了，而是直接得到每个pixel的normal vector，然后把它和light均仿射变换（affine mapping）后求出放射变换后的intensity，他即使该纹理坐标(u,v)的intensity。注意使用矩阵 M 变换light， 矩阵 $(M^{-1})^{T}$ 变换 normal vector。
      1. M = Projection * ModelView
    3. normal mapping需要用到**Transformation of normal vectors**：
      - 如果一个模型和他的法线都被定义在了obj file里，当我们需要用affine mapping来变换这个obj时，假设我们使用的是矩阵 M ，那么要变换这个obj的法线，我们需要使用的矩阵是$(M^{-1})^{T}$，即 M.inverse.transpose。
    4. global (Cartesian) coordinate system VS. Darboux frame (so-called tangent space)（6bis中学习）
2. 完善项目构架：
   1. For Gouraud Shading：
      1. vertex shader
         1. 用于取得顶点坐标，并转化为screenCoords
         2. 用于取得每个顶点的intensity、
         3. 用于取得每个顶点对应的纹理坐标(u,v)
      2. fragment shader
         1. 用于处理插值（得到P点的插值后的intensity，插值后的纹理坐标），并取得颜色数据
   2. For Normal Mapping：
      1. vertex shader：
         1. 用于取得顶点坐标，并转化为screenCoords
         2. 用于取得每个顶点对应的纹理坐标(u,v)
      2. fragment shader：
         1. 用于处理插值
            1. 得到P点插值后的纹理坐标（uP, vP), 然后把normal map（法线贴图）中（uP, vP)位置的（r,g,b)数据返回得到Original法线n_original。然后用 M.inverse.transpose仿射变换把n_original变成n（记得normalize）
            2. 用 M 把light_original变成light（记得normalize）
            3.  intensity/diffuse = max(0, n * l);
3. **intensity就是diffuse！！！！**
4. 学习Blinn Phong Reflection Model：
   1. diffuse = intensity = coefficients（即$\frac{intensity}{r^2}$） * max (0, n * l)。当coe = 1时，光线全部漫反射出去
   2. 直接用specular贴图得到specular的power的值, power = model->specular(uv)：然后用r计算specular：
      - 公式一 （Phong）： specular= coefficients, (即 $\frac{intensity}{r^2}$) * $(max(0, v * r))^p$。其中$ r = 2(n * l) - l$, n, l 均是normal vector， p一般选100～200(此处我们用specular map得到p)
      - 公式二（Blinn-Phong）：specular = coefficients (即 $\frac{intensity}{r^2}$) * $(max(0, n * h))^p$。其中$ h = \frac{l + e(view)}{|l + e(view)|}$, p一般选100～200(此处我们用specular map得到p)
   3. ambient = constant
   4. $final model = color[i] (即K) * (coef*diffuse + coef*specular + ambient)$; 一般diff+specular+ambient = 1？ 但是本project中ambient选了5，且不乘color[i]
5. 学习渲染管线：
   1. OpenGL 2 渲染管线如下图：In newer versions of OpenGL there are other shaders, allowing, for example, to generate geometry on the fly.
   - ![pipeline](LESSON/img/Pipeline.png)
   2. in the above image all the stages we can not touch are shown in blue, whereas our callbacks are shown in orange. In fact, our main() function - is the primitive processing routine. It calls the vertex shader. We do not have primitive assembly here, since we are drawing dumb triangles only (in our code it is merged with the primitive processing). triangle() function - is the rasterizer, for each point inside the triangle it calls the fragment shader, then performs depth checks (z-buffer) and such.
6. 跳过了Phong Shading

### 难题：
1. Phong Shading与Gouraud Shading的区别？之前用的intensity的插值方式究竟是Phong shading还是Gouraud shading？
   1. Phong Shading: 在vertex shader求出每个顶点的法线，然后fragment shader为法线插值，然后用得到的点P的法线求intensity。效果如下图：
   ![Phong Shading](LESSON/img/PhongShading.png)
   2. Gouraud Shading：在vertex shader中求出每个顶点的颜色（或者intensity），然后fragment shader只需为intensity插值（甚至可以直接在vertex shader里插值完后传给fragment shader），然后返回颜色
2. 更改后的geometry该怎么用？

## Lesson 6bis: tangent space normal mapping
### 主要内容：
1. normal mapping：
   1. Normal Mapping VS. Phong shading：Key Difference： the density of information we have
      1. Phong shading we use normal vectors given per vertex of triangle mesh (and interpolate it inside triangles)。
      2. Normal mapping textures provide dense information, dramatically improving rendering details

## Lesson 7: Shadow mapping
### 主要内容：
1. hard shadow：
   1. 做两次rendering：
      1. 第一次render把相机放在光源的位置，并把深度存放在shadow buffer中；此时的ModelView中的eye被替换为了light_dir，projection中的$ \frac{-1.f}{eye-center}$被替换为了0。
      2. 第二次render时把相机放在正常的位置上，新增一个uniform_Mshadow把这一次render中pixel的点坐标screenCoordCurr转换为上一次以光源为相机位置时pixel的点坐标screenCoordLight，（uniform_Mshadow的推导过程看下面)。然后比较screenCoordLight.z 与 shadowBuffer[screenCoordLight.x][screenCoordLight.y]，如果screenCoordLight.z大于shadowBuffer中的z值，说明当前物体离摄像头更近。我们返回shadow 值 1，即无shadow（最终的``color[i] = std::min<float>(20 + c[i]*shadow*(1.2*diff + .6*spec), 255)``。shadow对运算结果没有影响）, 否则返回shadow值0.3（即有shadow）：`` float shadow = .3+.7*(shadowbuffer[screenCoordLight.x][screenCoordLight.y] < creenCoordLight[2]);``。
         - 要得到uniform_Mshadow矩阵，首先保存之前在把相机放在光源上时得到的变换矩阵 $M_{shadow}$ = (Viewport_l * Projection_l * ModelView_l)。
         - 然后得到当前render的变换矩阵 $M_{Tran}$ = (Viewport * Projection * ModelView)。
         - 已知$M_{Tran}$可以把worldCoord变成当前相机位置下的screenCoord，那么$M_{Tran}^{-1}$就可以把screenCoord再变回worldCoord，此时再apply M_shadow就可以把worldCoord变成以光源为相机位置时的screenCoord。
2. z-fighting: 有时候由于两个物品的z值相差太小，距离太过接近，因此数据精度无法区分他们，从而导致两个物体z值计算错误
   - resolution: brute force solutions, 直接在比较的时候+43.34 ``float shadow = .3+.7*(shadowbuffer[idx]<sb_p[2]+43.34); // magic coeff to avoid z-fighting``

### 问题：
1. shadowbuffer的二维表示会导致segment fault：由于在main里定义了local的shadowBuffer，所以fragment里的shadowBuffer是空的
2. 为什么screenCoordLight.z大于shadowBuffer中的z值表示没有阴影，不是应该说明当前物体被更近的物体挡住了，我们返回shadow吗：
   - 不，如果screenCoordLight.z大于shadowBuffer中的z值，说明当前物体离摄像头更近。我们返回shadow 值 1（参考Lesson 3）
3. 阴影不完整是否是因为之前depth对不上
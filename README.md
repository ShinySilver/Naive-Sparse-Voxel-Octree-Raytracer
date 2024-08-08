Yet another voxel raytracing engine, using an SVO and OpenGL compute shaders.

![image](https://github.com/user-attachments/assets/cc564e6b-0b1d-46a6-a4f4-5c78500d1eee)

This raytracer is much better than my previous naive voxel engine that was using rasterization, but the memory footprint of my acceleration structure is horribly high, which limits its performance.

A short video of me playing with level of detail:
![Video](https://i.imgur.com/2vYmHsx.mp4)

And another illustrating the sparse voxel octree itself, with each bottom level acceleration structure having a unique color for visualization:
![Video](https://i.imgur.com/dVqGZiQ.mp4)

A similar view, but with a real terrain rather than a weird slope:
![image](https://github.com/user-attachments/assets/e8095f24-6755-4c49-a608-52bb2000d3c3)

I'll be archiving it soon as I want to change the acceleration structure which means changing most of the code, and I feel like it will be faster to restart from scratch :)
All in all, this project made me learn a lot on a lot of programmed-related things, and removed some dust from my algebra skills.
And most importantly, I had a lot of fun designing, and debugging everything. See below for random screenshots taken during debugging. 100% would recommend.

![image](https://github.com/user-attachments/assets/294763f3-6e20-4e07-8eab-78ddf10c9567)
![image](https://github.com/user-attachments/assets/6094a50d-e03b-4e81-8724-756495c32945)
![image](https://github.com/user-attachments/assets/98c7c67a-5c4f-4c99-ba8b-b01a06973475)
![image](https://github.com/user-attachments/assets/033ab208-b519-40ec-9b75-f65759af57d0)

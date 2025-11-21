#version 330 core
in vec3 fColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(fColor, 1.0);   // 상수색 쓰면 무조건 한 색만 나옴
}

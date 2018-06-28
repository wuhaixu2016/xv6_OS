#define YELLOW 0x0e00 // 类型
#define RED 0x0c00  // 括号
#define BLACK 0x0000    // 背景
#define WHITE 0x0f00    // 字体

int combineColor(int textColor, int backgroundColor)//设置字体的颜色和背景的颜色
{
    return ( textColor | ( backgroundColor << 4 ) );
}
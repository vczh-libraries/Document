#include "Util.h"

#include "TestOverloadingOperator_Input1.h"
#include "TestOverloadingOperator_Input2.h"
#include "TestOverloadingOperator_Input3.h"
#include "TestOverloadingOperator_Input4.h"

WString LoadOverloadingOperatorCode()
{
	FilePath input1 = L"../UnitTest/TestOverloadingOperator_Input1.h";
	FilePath input2 = L"../UnitTest/TestOverloadingOperator_Input2.h";
	FilePath input3 = L"../UnitTest/TestOverloadingOperator_Input3.h";
	FilePath input4 = L"../UnitTest/TestOverloadingOperator_Input4.h";

	WString code1, code2, code3, code4;
	TEST_ASSERT(File(input1).ReadAllTextByBom(code1));
	TEST_ASSERT(File(input2).ReadAllTextByBom(code2));
	TEST_ASSERT(File(input3).ReadAllTextByBom(code3));
	TEST_ASSERT(File(input4).ReadAllTextByBom(code4));

	return code1 + L"\r\n" + code2 + L"\r\n" + code3 + L"\r\n" + code4;
}

TEST_FILE
{
	TEST_CATEGORY(L"Postfix unary operators")
	{
		COMPILE_PROGRAM(program, pa, LoadOverloadingOperatorCode().Buffer());

		ASSERT_OVERLOADING(x++,									L"(x ++)",								void *);
		ASSERT_OVERLOADING(x--,									L"(x --)",								void *);
		ASSERT_OVERLOADING(y++,									L"(y ++)",								void *);
		ASSERT_OVERLOADING(y--,									L"(y --)",								void *);
		ASSERT_OVERLOADING(z++,									L"(z ++)",								void *);
		ASSERT_OVERLOADING(z--,									L"(z --)",								void *);

		ASSERT_OVERLOADING(cx++,								L"(cx ++)",								bool *);
		ASSERT_OVERLOADING(cx--,								L"(cx --)",								bool *);
		ASSERT_OVERLOADING(cy++,								L"(cy ++)",								bool *);
		ASSERT_OVERLOADING(cy--,								L"(cy --)",								bool *);
		ASSERT_OVERLOADING(cz++,								L"(cz ++)",								bool *);
		ASSERT_OVERLOADING(cz--,								L"(cz --)",								bool *);
	});

	TEST_CATEGORY(L"Prefix unary operators")
	{
		COMPILE_PROGRAM(program, pa, LoadOverloadingOperatorCode().Buffer());

		ASSERT_OVERLOADING(++x,									L"(++ x)",								void *);
		ASSERT_OVERLOADING(--x,									L"(-- x)",								void *);
		ASSERT_OVERLOADING(~x,									L"(~ x)",								void *);
		ASSERT_OVERLOADING(!x,									L"(! x)",								void *);
		ASSERT_OVERLOADING(-x,									L"(- x)",								void *);
		ASSERT_OVERLOADING(+x,									L"(+ x)",								void *);
		ASSERT_OVERLOADING(&x,									L"(& x)",								void *);
		ASSERT_OVERLOADING(*x,									L"(* x)",								void *);

		ASSERT_OVERLOADING(++y,									L"(++ y)",								void *);
		ASSERT_OVERLOADING(--y,									L"(-- y)",								void *);
		ASSERT_OVERLOADING(~y,									L"(~ y)",								void *);
		ASSERT_OVERLOADING(!y,									L"(! y)",								void *);
		ASSERT_OVERLOADING(-y,									L"(- y)",								void *);
		ASSERT_OVERLOADING(+y,									L"(+ y)",								void *);
		ASSERT_OVERLOADING(&y,									L"(& y)",								void *);
		ASSERT_OVERLOADING(*y,									L"(* y)",								void *);

		ASSERT_OVERLOADING(++z,									L"(++ z)",								void *);
		ASSERT_OVERLOADING(--z,									L"(-- z)",								void *);
		ASSERT_OVERLOADING(~z,									L"(~ z)",								void *);
		ASSERT_OVERLOADING(!z,									L"(! z)",								void *);
		ASSERT_OVERLOADING(-z,									L"(- z)",								void *);
		ASSERT_OVERLOADING(+z,									L"(+ z)",								void *);
		ASSERT_OVERLOADING(&z,									L"(& z)",								void *);
		ASSERT_OVERLOADING(*z,									L"(* z)",								void *);

		ASSERT_OVERLOADING(++cx,								L"(++ cx)",								bool *);
		ASSERT_OVERLOADING(--cx,								L"(-- cx)",								bool *);
		ASSERT_OVERLOADING(~cx,									L"(~ cx)",								bool *);
		ASSERT_OVERLOADING(!cx,									L"(! cx)",								bool *);
		ASSERT_OVERLOADING(-cx,									L"(- cx)",								bool *);
		ASSERT_OVERLOADING(+cx,									L"(+ cx)",								bool *);
		ASSERT_OVERLOADING(&cx,									L"(& cx)",								bool *);
		ASSERT_OVERLOADING(*cx,									L"(* cx)",								bool *);

		ASSERT_OVERLOADING(++cy,								L"(++ cy)",								bool *);
		ASSERT_OVERLOADING(--cy,								L"(-- cy)",								bool *);
		ASSERT_OVERLOADING(~cy,									L"(~ cy)",								bool *);
		ASSERT_OVERLOADING(!cy,									L"(! cy)",								bool *);
		ASSERT_OVERLOADING(-cy,									L"(- cy)",								bool *);
		ASSERT_OVERLOADING(+cy,									L"(+ cy)",								bool *);
		ASSERT_OVERLOADING(&cy,									L"(& cy)",								bool *);
		ASSERT_OVERLOADING(*cy,									L"(* cy)",								bool *);

		ASSERT_OVERLOADING(++cz,								L"(++ cz)",								bool *);
		ASSERT_OVERLOADING(--cz,								L"(-- cz)",								bool *);
		ASSERT_OVERLOADING(~cz,									L"(~ cz)",								bool *);
		ASSERT_OVERLOADING(!cz,									L"(! cz)",								bool *);
		ASSERT_OVERLOADING(-cz,									L"(- cz)",								bool *);
		ASSERT_OVERLOADING(+cz,									L"(+ cz)",								bool *);
		ASSERT_OVERLOADING(&cz,									L"(& cz)",								bool *);
		ASSERT_OVERLOADING(*cz,									L"(* cz)",								bool *);
	});

#define _ ,

	TEST_CATEGORY(L"Binary operators")
	{
		COMPILE_PROGRAM(program, pa, LoadOverloadingOperatorCode().Buffer());

		ASSERT_OVERLOADING(x* 0,								L"(x * 0)",								void *);
		ASSERT_OVERLOADING(x/ 0,								L"(x / 0)",								void *);
		ASSERT_OVERLOADING(x% 0,								L"(x % 0)",								void *);
		ASSERT_OVERLOADING(x+ 0,								L"(x + 0)",								void *);
		ASSERT_OVERLOADING(x- 0,								L"(x - 0)",								void *);
		ASSERT_OVERLOADING(x<<0,								L"(x << 0)",							void *);
		ASSERT_OVERLOADING(x>>0,								L"(x >> 0)",							void *);
		ASSERT_OVERLOADING(x==0,								L"(x == 0)",							void *);
		ASSERT_OVERLOADING(x!=0,								L"(x != 0)",							void *);
		ASSERT_OVERLOADING(x< 0,								L"(x < 0)",								void *);
		ASSERT_OVERLOADING(x<=0,								L"(x <= 0)",							void *);
		ASSERT_OVERLOADING(x> 0,								L"(x > 0)",								void *);
		ASSERT_OVERLOADING(x>=0,								L"(x >= 0)",							void *);
		ASSERT_OVERLOADING(x& 0,								L"(x & 0)",								void *);
		ASSERT_OVERLOADING(x| 0,								L"(x | 0)",								void *);
		ASSERT_OVERLOADING(x^ 0,								L"(x ^ 0)",								void *);
		ASSERT_OVERLOADING(x&&0,								L"(x && 0)",							void *);
		ASSERT_OVERLOADING(x||0,								L"(x || 0)",							void *);
		ASSERT_OVERLOADING(x= 0,								L"(x = 0)",								void *);
		ASSERT_OVERLOADING(x*=0,								L"(x *= 0)",							void *);
		ASSERT_OVERLOADING(x/=0,								L"(x /= 0)",							void *);
		ASSERT_OVERLOADING(x%=0,								L"(x %= 0)",							void *);
		ASSERT_OVERLOADING(x+=0,								L"(x += 0)",							void *);
		ASSERT_OVERLOADING(x-=0,								L"(x -= 0)",							void *);
		ASSERT_OVERLOADING(x<<0,								L"(x << 0)",							void *);
		ASSERT_OVERLOADING(x>>0,								L"(x >> 0)",							void *);
		ASSERT_OVERLOADING(x&=0,								L"(x &= 0)",							void *);
		ASSERT_OVERLOADING(x|=0,								L"(x |= 0)",							void *);
		ASSERT_OVERLOADING(x^=0,								L"(x ^= 0)",							void *);
		ASSERT_OVERLOADING(x^=0,								L"(x ^= 0)",							void *);
		ASSERT_OVERLOADING(x _ 0,								L"(x , 0)",								void *);

		ASSERT_OVERLOADING(y* 0,								L"(y * 0)",								void *);
		ASSERT_OVERLOADING(y/ 0,								L"(y / 0)",								void *);
		ASSERT_OVERLOADING(y% 0,								L"(y % 0)",								void *);
		ASSERT_OVERLOADING(y+ 0,								L"(y + 0)",								void *);
		ASSERT_OVERLOADING(y- 0,								L"(y - 0)",								void *);
		ASSERT_OVERLOADING(y<<0,								L"(y << 0)",							void *);
		ASSERT_OVERLOADING(y>>0,								L"(y >> 0)",							void *);
		ASSERT_OVERLOADING(y==0,								L"(y == 0)",							void *);
		ASSERT_OVERLOADING(y!=0,								L"(y != 0)",							void *);
		ASSERT_OVERLOADING(y< 0,								L"(y < 0)",								void *);
		ASSERT_OVERLOADING(y<=0,								L"(y <= 0)",							void *);
		ASSERT_OVERLOADING(y> 0,								L"(y > 0)",								void *);
		ASSERT_OVERLOADING(y>=0,								L"(y >= 0)",							void *);
		ASSERT_OVERLOADING(y& 0,								L"(y & 0)",								void *);
		ASSERT_OVERLOADING(y| 0,								L"(y | 0)",								void *);
		ASSERT_OVERLOADING(y^ 0,								L"(y ^ 0)",								void *);
		ASSERT_OVERLOADING(y&&0,								L"(y && 0)",							void *);
		ASSERT_OVERLOADING(y||0,								L"(y || 0)",							void *);
		ASSERT_OVERLOADING(y*=0,								L"(y *= 0)",							void *);
		ASSERT_OVERLOADING(y/=0,								L"(y /= 0)",							void *);
		ASSERT_OVERLOADING(y%=0,								L"(y %= 0)",							void *);
		ASSERT_OVERLOADING(y+=0,								L"(y += 0)",							void *);
		ASSERT_OVERLOADING(y-=0,								L"(y -= 0)",							void *);
		ASSERT_OVERLOADING(y<<0,								L"(y << 0)",							void *);
		ASSERT_OVERLOADING(y>>0,								L"(y >> 0)",							void *);
		ASSERT_OVERLOADING(y&=0,								L"(y &= 0)",							void *);
		ASSERT_OVERLOADING(y|=0,								L"(y |= 0)",							void *);
		ASSERT_OVERLOADING(y^=0,								L"(y ^= 0)",							void *);
		ASSERT_OVERLOADING(y _ 0,								L"(y , 0)",								void *);

		ASSERT_OVERLOADING(0* y,								L"(0 * y)",								void *);
		ASSERT_OVERLOADING(0/ y,								L"(0 / y)",								void *);
		ASSERT_OVERLOADING(0% y,								L"(0 % y)",								void *);
		ASSERT_OVERLOADING(0+ y,								L"(0 + y)",								void *);
		ASSERT_OVERLOADING(0- y,								L"(0 - y)",								void *);
		ASSERT_OVERLOADING(0<<y,								L"(0 << y)",							void *);
		ASSERT_OVERLOADING(0>>y,								L"(0 >> y)",							void *);
		ASSERT_OVERLOADING(0==y,								L"(0 == y)",							void *);
		ASSERT_OVERLOADING(0!=y,								L"(0 != y)",							void *);
		ASSERT_OVERLOADING(0< y,								L"(0 < y)",								void *);
		ASSERT_OVERLOADING(0<=y,								L"(0 <= y)",							void *);
		ASSERT_OVERLOADING(0> y,								L"(0 > y)",								void *);
		ASSERT_OVERLOADING(0>=y,								L"(0 >= y)",							void *);
		ASSERT_OVERLOADING(0& y,								L"(0 & y)",								void *);
		ASSERT_OVERLOADING(0| y,								L"(0 | y)",								void *);
		ASSERT_OVERLOADING(0^ y,								L"(0 ^ y)",								void *);
		ASSERT_OVERLOADING(0&&y,								L"(0 && y)",							void *);
		ASSERT_OVERLOADING(0||y,								L"(0 || y)",							void *);
		ASSERT_OVERLOADING(0*=y,								L"(0 *= y)",							void *);
		ASSERT_OVERLOADING(0/=y,								L"(0 /= y)",							void *);
		ASSERT_OVERLOADING(0%=y,								L"(0 %= y)",							void *);
		ASSERT_OVERLOADING(0+=y,								L"(0 += y)",							void *);
		ASSERT_OVERLOADING(0-=y,								L"(0 -= y)",							void *);
		ASSERT_OVERLOADING(0<<y,								L"(0 << y)",							void *);
		ASSERT_OVERLOADING(0>>y,								L"(0 >> y)",							void *);
		ASSERT_OVERLOADING(0&=y,								L"(0 &= y)",							void *);
		ASSERT_OVERLOADING(0|=y,								L"(0 |= y)",							void *);
		ASSERT_OVERLOADING(0^=y,								L"(0 ^= y)",							void *);
		ASSERT_OVERLOADING(0 _ y,								L"(0 , y)",								void *);

		ASSERT_OVERLOADING(z* 0,								L"(z * 0)",								void *);
		ASSERT_OVERLOADING(z/ 0,								L"(z / 0)",								void *);
		ASSERT_OVERLOADING(z% 0,								L"(z % 0)",								void *);
		ASSERT_OVERLOADING(z+ 0,								L"(z + 0)",								void *);
		ASSERT_OVERLOADING(z- 0,								L"(z - 0)",								void *);
		ASSERT_OVERLOADING(z<<0,								L"(z << 0)",							void *);
		ASSERT_OVERLOADING(z>>0,								L"(z >> 0)",							void *);
		ASSERT_OVERLOADING(z==0,								L"(z == 0)",							void *);
		ASSERT_OVERLOADING(z!=0,								L"(z != 0)",							void *);
		ASSERT_OVERLOADING(z< 0,								L"(z < 0)",								void *);
		ASSERT_OVERLOADING(z<=0,								L"(z <= 0)",							void *);
		ASSERT_OVERLOADING(z> 0,								L"(z > 0)",								void *);
		ASSERT_OVERLOADING(z>=0,								L"(z >= 0)",							void *);
		ASSERT_OVERLOADING(z& 0,								L"(z & 0)",								void *);
		ASSERT_OVERLOADING(z| 0,								L"(z | 0)",								void *);
		ASSERT_OVERLOADING(z^ 0,								L"(z ^ 0)",								void *);
		ASSERT_OVERLOADING(z&&0,								L"(z && 0)",							void *);
		ASSERT_OVERLOADING(z||0,								L"(z || 0)",							void *);
		ASSERT_OVERLOADING(z*=0,								L"(z *= 0)",							void *);
		ASSERT_OVERLOADING(z/=0,								L"(z /= 0)",							void *);
		ASSERT_OVERLOADING(z%=0,								L"(z %= 0)",							void *);
		ASSERT_OVERLOADING(z+=0,								L"(z += 0)",							void *);
		ASSERT_OVERLOADING(z-=0,								L"(z -= 0)",							void *);
		ASSERT_OVERLOADING(z<<0,								L"(z << 0)",							void *);
		ASSERT_OVERLOADING(z>>0,								L"(z >> 0)",							void *);
		ASSERT_OVERLOADING(z&=0,								L"(z &= 0)",							void *);
		ASSERT_OVERLOADING(z|=0,								L"(z |= 0)",							void *);
		ASSERT_OVERLOADING(z^=0,								L"(z ^= 0)",							void *);
		ASSERT_OVERLOADING(z _ 0,								L"(z , 0)",								void *);

		ASSERT_OVERLOADING(0* z,								L"(0 * z)",								void *);
		ASSERT_OVERLOADING(0/ z,								L"(0 / z)",								void *);
		ASSERT_OVERLOADING(0% z,								L"(0 % z)",								void *);
		ASSERT_OVERLOADING(0+ z,								L"(0 + z)",								void *);
		ASSERT_OVERLOADING(0- z,								L"(0 - z)",								void *);
		ASSERT_OVERLOADING(0<<z,								L"(0 << z)",							void *);
		ASSERT_OVERLOADING(0>>z,								L"(0 >> z)",							void *);
		ASSERT_OVERLOADING(0==z,								L"(0 == z)",							void *);
		ASSERT_OVERLOADING(0!=z,								L"(0 != z)",							void *);
		ASSERT_OVERLOADING(0< z,								L"(0 < z)",								void *);
		ASSERT_OVERLOADING(0<=z,								L"(0 <= z)",							void *);
		ASSERT_OVERLOADING(0> z,								L"(0 > z)",								void *);
		ASSERT_OVERLOADING(0>=z,								L"(0 >= z)",							void *);
		ASSERT_OVERLOADING(0& z,								L"(0 & z)",								void *);
		ASSERT_OVERLOADING(0| z,								L"(0 | z)",								void *);
		ASSERT_OVERLOADING(0^ z,								L"(0 ^ z)",								void *);
		ASSERT_OVERLOADING(0&&z,								L"(0 && z)",							void *);
		ASSERT_OVERLOADING(0||z,								L"(0 || z)",							void *);
		ASSERT_OVERLOADING(0*=z,								L"(0 *= z)",							void *);
		ASSERT_OVERLOADING(0/=z,								L"(0 /= z)",							void *);
		ASSERT_OVERLOADING(0%=z,								L"(0 %= z)",							void *);
		ASSERT_OVERLOADING(0+=z,								L"(0 += z)",							void *);
		ASSERT_OVERLOADING(0-=z,								L"(0 -= z)",							void *);
		ASSERT_OVERLOADING(0<<z,								L"(0 << z)",							void *);
		ASSERT_OVERLOADING(0>>z,								L"(0 >> z)",							void *);
		ASSERT_OVERLOADING(0&=z,								L"(0 &= z)",							void *);
		ASSERT_OVERLOADING(0|=z,								L"(0 |= z)",							void *);
		ASSERT_OVERLOADING(0^=z,								L"(0 ^= z)",							void *);
		ASSERT_OVERLOADING(0 _ z,								L"(0 , z)",								void *);

		ASSERT_OVERLOADING(cx* 0,								L"(cx * 0)",							bool *);
		ASSERT_OVERLOADING(cx/ 0,								L"(cx / 0)",							bool *);
		ASSERT_OVERLOADING(cx% 0,								L"(cx % 0)",							bool *);
		ASSERT_OVERLOADING(cx+ 0,								L"(cx + 0)",							bool *);
		ASSERT_OVERLOADING(cx- 0,								L"(cx - 0)",							bool *);
		ASSERT_OVERLOADING(cx<<0,								L"(cx << 0)",							bool *);
		ASSERT_OVERLOADING(cx>>0,								L"(cx >> 0)",							bool *);
		ASSERT_OVERLOADING(cx==0,								L"(cx == 0)",							bool *);
		ASSERT_OVERLOADING(cx!=0,								L"(cx != 0)",							bool *);
		ASSERT_OVERLOADING(cx< 0,								L"(cx < 0)",							bool *);
		ASSERT_OVERLOADING(cx<=0,								L"(cx <= 0)",							bool *);
		ASSERT_OVERLOADING(cx> 0,								L"(cx > 0)",							bool *);
		ASSERT_OVERLOADING(cx>=0,								L"(cx >= 0)",							bool *);
		ASSERT_OVERLOADING(cx& 0,								L"(cx & 0)",							bool *);
		ASSERT_OVERLOADING(cx| 0,								L"(cx | 0)",							bool *);
		ASSERT_OVERLOADING(cx^ 0,								L"(cx ^ 0)",							bool *);
		ASSERT_OVERLOADING(cx&&0,								L"(cx && 0)",							bool *);
		ASSERT_OVERLOADING(cx||0,								L"(cx || 0)",							bool *);
		ASSERT_OVERLOADING(cx= 0,								L"(cx = 0)",							bool *);
		ASSERT_OVERLOADING(cx*=0,								L"(cx *= 0)",							bool *);
		ASSERT_OVERLOADING(cx/=0,								L"(cx /= 0)",							bool *);
		ASSERT_OVERLOADING(cx%=0,								L"(cx %= 0)",							bool *);
		ASSERT_OVERLOADING(cx+=0,								L"(cx += 0)",							bool *);
		ASSERT_OVERLOADING(cx-=0,								L"(cx -= 0)",							bool *);
		ASSERT_OVERLOADING(cx<<0,								L"(cx << 0)",							bool *);
		ASSERT_OVERLOADING(cx>>0,								L"(cx >> 0)",							bool *);
		ASSERT_OVERLOADING(cx&=0,								L"(cx &= 0)",							bool *);
		ASSERT_OVERLOADING(cx|=0,								L"(cx |= 0)",							bool *);
		ASSERT_OVERLOADING(cx^=0,								L"(cx ^= 0)",							bool *);
		ASSERT_OVERLOADING(cx _ 0,								L"(cx , 0)",							bool *);

		ASSERT_OVERLOADING(cy* 0,								L"(cy * 0)",							bool *);
		ASSERT_OVERLOADING(cy/ 0,								L"(cy / 0)",							bool *);
		ASSERT_OVERLOADING(cy% 0,								L"(cy % 0)",							bool *);
		ASSERT_OVERLOADING(cy+ 0,								L"(cy + 0)",							bool *);
		ASSERT_OVERLOADING(cy- 0,								L"(cy - 0)",							bool *);
		ASSERT_OVERLOADING(cy<<0,								L"(cy << 0)",							bool *);
		ASSERT_OVERLOADING(cy>>0,								L"(cy >> 0)",							bool *);
		ASSERT_OVERLOADING(cy==0,								L"(cy == 0)",							bool *);
		ASSERT_OVERLOADING(cy!=0,								L"(cy != 0)",							bool *);
		ASSERT_OVERLOADING(cy< 0,								L"(cy < 0)",							bool *);
		ASSERT_OVERLOADING(cy<=0,								L"(cy <= 0)",							bool *);
		ASSERT_OVERLOADING(cy> 0,								L"(cy > 0)",							bool *);
		ASSERT_OVERLOADING(cy>=0,								L"(cy >= 0)",							bool *);
		ASSERT_OVERLOADING(cy& 0,								L"(cy & 0)",							bool *);
		ASSERT_OVERLOADING(cy| 0,								L"(cy | 0)",							bool *);
		ASSERT_OVERLOADING(cy^ 0,								L"(cy ^ 0)",							bool *);
		ASSERT_OVERLOADING(cy&&0,								L"(cy && 0)",							bool *);
		ASSERT_OVERLOADING(cy||0,								L"(cy || 0)",							bool *);
		ASSERT_OVERLOADING(cy*=0,								L"(cy *= 0)",							bool *);
		ASSERT_OVERLOADING(cy/=0,								L"(cy /= 0)",							bool *);
		ASSERT_OVERLOADING(cy%=0,								L"(cy %= 0)",							bool *);
		ASSERT_OVERLOADING(cy+=0,								L"(cy += 0)",							bool *);
		ASSERT_OVERLOADING(cy-=0,								L"(cy -= 0)",							bool *);
		ASSERT_OVERLOADING(cy<<0,								L"(cy << 0)",							bool *);
		ASSERT_OVERLOADING(cy>>0,								L"(cy >> 0)",							bool *);
		ASSERT_OVERLOADING(cy&=0,								L"(cy &= 0)",							bool *);
		ASSERT_OVERLOADING(cy|=0,								L"(cy |= 0)",							bool *);
		ASSERT_OVERLOADING(cy^=0,								L"(cy ^= 0)",							bool *);
		ASSERT_OVERLOADING(cy _ 0,								L"(cy , 0)",							bool *);

		ASSERT_OVERLOADING(0* cy,								L"(0 * cy)",							bool *);
		ASSERT_OVERLOADING(0/ cy,								L"(0 / cy)",							bool *);
		ASSERT_OVERLOADING(0% cy,								L"(0 % cy)",							bool *);
		ASSERT_OVERLOADING(0+ cy,								L"(0 + cy)",							bool *);
		ASSERT_OVERLOADING(0- cy,								L"(0 - cy)",							bool *);
		ASSERT_OVERLOADING(0<<cy,								L"(0 << cy)",							bool *);
		ASSERT_OVERLOADING(0>>cy,								L"(0 >> cy)",							bool *);
		ASSERT_OVERLOADING(0==cy,								L"(0 == cy)",							bool *);
		ASSERT_OVERLOADING(0!=cy,								L"(0 != cy)",							bool *);
		ASSERT_OVERLOADING(0< cy,								L"(0 < cy)",							bool *);
		ASSERT_OVERLOADING(0<=cy,								L"(0 <= cy)",							bool *);
		ASSERT_OVERLOADING(0> cy,								L"(0 > cy)",							bool *);
		ASSERT_OVERLOADING(0>=cy,								L"(0 >= cy)",							bool *);
		ASSERT_OVERLOADING(0& cy,								L"(0 & cy)",							bool *);
		ASSERT_OVERLOADING(0| cy,								L"(0 | cy)",							bool *);
		ASSERT_OVERLOADING(0^ cy,								L"(0 ^ cy)",							bool *);
		ASSERT_OVERLOADING(0&&cy,								L"(0 && cy)",							bool *);
		ASSERT_OVERLOADING(0||cy,								L"(0 || cy)",							bool *);
		ASSERT_OVERLOADING(0*=cy,								L"(0 *= cy)",							bool *);
		ASSERT_OVERLOADING(0/=cy,								L"(0 /= cy)",							bool *);
		ASSERT_OVERLOADING(0%=cy,								L"(0 %= cy)",							bool *);
		ASSERT_OVERLOADING(0+=cy,								L"(0 += cy)",							bool *);
		ASSERT_OVERLOADING(0-=cy,								L"(0 -= cy)",							bool *);
		ASSERT_OVERLOADING(0<<cy,								L"(0 << cy)",							bool *);
		ASSERT_OVERLOADING(0>>cy,								L"(0 >> cy)",							bool *);
		ASSERT_OVERLOADING(0&=cy,								L"(0 &= cy)",							bool *);
		ASSERT_OVERLOADING(0|=cy,								L"(0 |= cy)",							bool *);
		ASSERT_OVERLOADING(0^=cy,								L"(0 ^= cy)",							bool *);
		ASSERT_OVERLOADING(0 _ cy,								L"(0 , cy)",							bool *);

		ASSERT_OVERLOADING(cz* 0,								L"(cz * 0)",							bool *);
		ASSERT_OVERLOADING(cz/ 0,								L"(cz / 0)",							bool *);
		ASSERT_OVERLOADING(cz% 0,								L"(cz % 0)",							bool *);
		ASSERT_OVERLOADING(cz+ 0,								L"(cz + 0)",							bool *);
		ASSERT_OVERLOADING(cz- 0,								L"(cz - 0)",							bool *);
		ASSERT_OVERLOADING(cz<<0,								L"(cz << 0)",							bool *);
		ASSERT_OVERLOADING(cz>>0,								L"(cz >> 0)",							bool *);
		ASSERT_OVERLOADING(cz==0,								L"(cz == 0)",							bool *);
		ASSERT_OVERLOADING(cz!=0,								L"(cz != 0)",							bool *);
		ASSERT_OVERLOADING(cz< 0,								L"(cz < 0)",							bool *);
		ASSERT_OVERLOADING(cz<=0,								L"(cz <= 0)",							bool *);
		ASSERT_OVERLOADING(cz> 0,								L"(cz > 0)",							bool *);
		ASSERT_OVERLOADING(cz>=0,								L"(cz >= 0)",							bool *);
		ASSERT_OVERLOADING(cz& 0,								L"(cz & 0)",							bool *);
		ASSERT_OVERLOADING(cz| 0,								L"(cz | 0)",							bool *);
		ASSERT_OVERLOADING(cz^ 0,								L"(cz ^ 0)",							bool *);
		ASSERT_OVERLOADING(cz&&0,								L"(cz && 0)",							bool *);
		ASSERT_OVERLOADING(cz||0,								L"(cz || 0)",							bool *);
		ASSERT_OVERLOADING(cz*=0,								L"(cz *= 0)",							bool *);
		ASSERT_OVERLOADING(cz/=0,								L"(cz /= 0)",							bool *);
		ASSERT_OVERLOADING(cz%=0,								L"(cz %= 0)",							bool *);
		ASSERT_OVERLOADING(cz+=0,								L"(cz += 0)",							bool *);
		ASSERT_OVERLOADING(cz-=0,								L"(cz -= 0)",							bool *);
		ASSERT_OVERLOADING(cz<<0,								L"(cz << 0)",							bool *);
		ASSERT_OVERLOADING(cz>>0,								L"(cz >> 0)",							bool *);
		ASSERT_OVERLOADING(cz&=0,								L"(cz &= 0)",							bool *);
		ASSERT_OVERLOADING(cz|=0,								L"(cz |= 0)",							bool *);
		ASSERT_OVERLOADING(cz^=0,								L"(cz ^= 0)",							bool *);
		ASSERT_OVERLOADING(cz _ 0,								L"(cz , 0)",							bool *);

		ASSERT_OVERLOADING(0* cz,								L"(0 * cz)",							bool *);
		ASSERT_OVERLOADING(0/ cz,								L"(0 / cz)",							bool *);
		ASSERT_OVERLOADING(0% cz,								L"(0 % cz)",							bool *);
		ASSERT_OVERLOADING(0+ cz,								L"(0 + cz)",							bool *);
		ASSERT_OVERLOADING(0- cz,								L"(0 - cz)",							bool *);
		ASSERT_OVERLOADING(0<<cz,								L"(0 << cz)",							bool *);
		ASSERT_OVERLOADING(0>>cz,								L"(0 >> cz)",							bool *);
		ASSERT_OVERLOADING(0==cz,								L"(0 == cz)",							bool *);
		ASSERT_OVERLOADING(0!=cz,								L"(0 != cz)",							bool *);
		ASSERT_OVERLOADING(0< cz,								L"(0 < cz)",							bool *);
		ASSERT_OVERLOADING(0<=cz,								L"(0 <= cz)",							bool *);
		ASSERT_OVERLOADING(0> cz,								L"(0 > cz)",							bool *);
		ASSERT_OVERLOADING(0>=cz,								L"(0 >= cz)",							bool *);
		ASSERT_OVERLOADING(0& cz,								L"(0 & cz)",							bool *);
		ASSERT_OVERLOADING(0| cz,								L"(0 | cz)",							bool *);
		ASSERT_OVERLOADING(0^ cz,								L"(0 ^ cz)",							bool *);
		ASSERT_OVERLOADING(0&&cz,								L"(0 && cz)",							bool *);
		ASSERT_OVERLOADING(0||cz,								L"(0 || cz)",							bool *);
		ASSERT_OVERLOADING(0*=cz,								L"(0 *= cz)",							bool *);
		ASSERT_OVERLOADING(0/=cz,								L"(0 /= cz)",							bool *);
		ASSERT_OVERLOADING(0%=cz,								L"(0 %= cz)",							bool *);
		ASSERT_OVERLOADING(0+=cz,								L"(0 += cz)",							bool *);
		ASSERT_OVERLOADING(0-=cz,								L"(0 -= cz)",							bool *);
		ASSERT_OVERLOADING(0<<cz,								L"(0 << cz)",							bool *);
		ASSERT_OVERLOADING(0>>cz,								L"(0 >> cz)",							bool *);
		ASSERT_OVERLOADING(0&=cz,								L"(0 &= cz)",							bool *);
		ASSERT_OVERLOADING(0|=cz,								L"(0 |= cz)",							bool *);
		ASSERT_OVERLOADING(0^=cz,								L"(0 ^= cz)",							bool *);
		ASSERT_OVERLOADING(0 _ cz,								L"(0 , cz)",							bool *);
	});

#undef _
}
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define TRUE 1
#define FALSE 0
typedef unsigned char byte;

char mbuf[256];
int readmode;


/*ファイルより数バイト読み込み*/
int readData(char *data, int len, FILE *fp)
{
	return fread(data, len, sizeof(char), fp); //fpからlen長さのsizeof(char)分のデータを読み込む
}


/*４バイトの数値データをファイルより読み込む*/
int getLong(FILE *fp)
{
	unsigned char buf[10];

	readData(buf, 4, fp);
	return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

/*2バイトの数値データをファイルより読み込み*/
int getShort(FILE *fp)
{
	unsigned char buf[10];

	readData(buf,2,fp);
	return (buf[0] << 8) | buf[1];
}

/*1バイトを読み込む*/
unsigned int getC(FILE *fp)
{
	unsigned char buf[4];
	readData(buf, 1, fp);

	return (unsigned int)buf[0];
}


static int bits = 0;
static int bdata = 0;

/*入力処理の初期化*/
int fgetBitInit()
{
	bits = 0;
	bdata = 0;
	return 0;
}

/*1バイト入力*/
int fgetBit(FILE *fp)
{
	unsigned char bbuf;
	int val;

	if(bits == 0){
		readData(&bbuf, 1, fp);
		bdata = bbuf;
	}

	val = (bdata >> 7)  & 1;
	bdata = (bdata << 1) & 0xff;
	bits++;
	if(bits >= 8){
		bits = 0;
		bdata = 0;
	}
	return val;
}

/*ファイルよりnビット読み込み*/
int fgetBits(FILE *fp, int bits)
{
	int i,res;

	res = 0;
	for(i = 0; i < bits; i++){
		res = res * 2;
		res += fgetBit(fp);
	}

	return res;
}

/*辞書バッファ*/
typedef int BUF;
typedef struct SDIC
{
	BUF *buffer;  				
	int dic_len, code_len;   	/*辞書サイズとコードサイズ*/
	int buf_size;				/*バッファ全体のサイズ*/
	int dic_bgn;				/*辞書部分の始まり*/	
	int code_bgn, code_end;     /*コード部分の始まりと終わり*/
}SlideDic;


/*スライド辞書中の次のデータ位置を求める*/
int nextPos(int now, int bsize)
{
	return (now + 1) % bsize;
}


/*スライド辞書の初期化*/
int initSlideDic(SlideDic *sdic, int dic, int code)
{
	int i;
	int size;

	size = dic + code;
	if((sdic->buffer = malloc(sizeof(BUF)*size)) == NULL)
		return FALSE;
	for(i = 0; i < size; i++)
		sdic->buffer[i] = 0;
	sdic->dic_len = dic;
	sdic->code_len = code;
	sdic->buf_size = size;
	sdic->dic_bgn = 0;
	sdic->code_bgn = 0;
	sdic->code_end = 0;

	return TRUE;
}

/*x1,x2間のスライド辞書上での距離を求める*/
int getDistance(int x1, int x2, int size)
{
	int sa;

	sa = x1 - x2;
	if(sa >= 0) return sa;
	return sa + size;
}

/*スライド辞書に１文字追加*/
int insertOneCodeToDic(SlideDic *sdic, int c)
{
	int bsize;
	int sa;

	bsize = sdic->buf_size;

	sdic->buffer[sdic->code_bgn] = c;
	sdic ->code_bgn = nextPos(sdic->code_bgn,bsize);
	sa = getDistance(sdic->code_bgn, sdic->dic_bgn, bsize);
	if(sa > sdic -> dic_len)
		sdic->dic_bgn = nextPos(sdic->dic_bgn, bsize);

	return 0;
}

/*2のx乗を求める*/
int getPow2(int x)
{
	int i;
	int s;

	s = 1;
	for (i = 0; i < x; i++) s *=2;
	return s;	
}


/*展開メイン処理*/
long decode(FILE *fp, int size, int dic_bits, int code_bits)
{
	SlideDic sdic;
	int p,c;
	int i,d_bgn;
	int flg,pos,len;
	int dic,code;

	dic = getPow2(dic_bits);
	code = getPow2(code_bits) + 2;

	p = 0;
	fgetBitInit();
	initSlideDic(&sdic, dic, code);
	while(p < size){
		flg = fgetBit(fp);
		if(flg){
			pos = fgetBits(fp,dic_bits);
			len = fgetBits(fp,code_bits) + 3;
//			printf("[%d %d]",pos,len);
			d_bgn = (sdic.dic_bgn + pos) % sdic.buf_size;
			for(i = 0; i < len; i++){
				c = sdic.buffer[d_bgn];
				printf("%c",c);
				p++;
				insertOneCodeToDic(&sdic,c);
				d_bgn = nextPos(d_bgn,sdic.buf_size);
			}
		}
		else{
			c = fgetBits(fp,8);
			printf("%c",c);
			p++;
			insertOneCodeToDic(&sdic,c);
		}
	}

	return 0;
}


/*Main関数*/
int main(int ac, char *av[])
{
	FILE *fp;
	int size = 0;
//	int prev = 0;
	int dic,code;

	if((fp = fopen(av[1], "rb")) == NULL)
		return 6;

	size = getLong(fp);
	dic = getLong(fp);
	code = getLong(fp);
//	printf("%d %d %d\n",size,dic,code);

	decode(fp,size,dic,code);
	return 0;
}






















































































#include <stdio.h>
#include <stdlib.h>
#include "encode.h"

int main(int argc, char **argv)
{
  if (argc == 2) { //読み込むファイルが1つだけ指定されている時は、encodeの第二引数をNULLにします
    encode(argv[1], NULL);
    return EXIT_SUCCESS;
  }else if (argc == 4) { //ファイル名2つとモードが指定されているときの処理です
    if(argv[3][0] == 'e'){ //モードがeの時、テキストファイルの圧縮を行います
      encode(argv[1], argv[2]);
      return EXIT_SUCCESS;
    }else if(argv[3][0] == 'd'){ //モードがdの時、圧縮ファイルの回答を行います
      decode(argv[1], argv[2]);
      return EXIT_SUCCESS;
    }
  }else{ //引数の数が合わない場合、使い方を表示します
    fprintf(stderr, "usage: %s <filename> or %s <filename1> <filename2> <mode(e or d)>\n",argv[0], argv[0]);
    exit(1);
  }
}

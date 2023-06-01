#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "encode.h"



static const int nsymbols = 256 + 1;
int * symbol_count;

typedef struct node
{
  int symbol;
  int count;
  struct node *left;
  struct node *right;
  int code; //このノードに対応するハフマン符号を10進数に直した数です
  int length; //対応するハフマン符号の長さです
} Node;



static void count_symbols(const char *filename);

static Node *create_node(int symbol, int count, Node *left, Node *right);

static Node *pop_min(int *n, Node *nodep[]);

static Node *build_tree(void);

static void traverse_tree(const int depth, Node *np, const int code, const int tree[], int *number_of_symbols); //treeは表示を整えるための配列です。dummyでないノードの数をnumber_of_symbolsに格納します

static void print_binary(int n, int depth); //10真数の数をdepth桁までの2進数にして表示します

static void node_search(Node *np, Node **ans, const int symbol); //npからsymbolを持つノードを探し、ansにそのポインタを格納します

static void write_header(const Node *root, FILE *fp); //ファイルに符号の数と文字と符号の対応を書き込みます

static void write_body(Node *root, FILE *fp1, FILE *fp2); //ファイルに本文部分を書き込みます

int decode(const char *filename1, const char *filename2); //符号化したファイルを解凍する関数です

static int search_symbol(const int code, const int length, const Node *code_list, const int number_of_symbols); //符号からそれに対応する文字を探す関数です

static void count_symbols(const char *filename)
{
  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    fprintf(stderr, "error: cannot open %s\n", filename);
    exit(1);
  }

  symbol_count = (int*)calloc(nsymbols, sizeof(int));
  
  int tmp;
  while(fread(&tmp, sizeof(char), 1, fp) == 1){
    symbol_count[tmp] += 1;
  }
  fclose(fp);
}

static Node *create_node(int symbol, int count, Node *left, Node *right)
{
  Node *ret = (Node *)malloc(sizeof(Node));
  *ret = (Node){ .symbol = symbol, .count = count, .left = left, .right = right};
  return ret;
}

static Node *pop_min(int *n, Node *nodep[])
{
  int argmin = 0;
  for (int i = 0; i < *n; i++) {
    if (nodep[i]->count < nodep[argmin]->count) {
      argmin = i;
    }
  }

  Node *node_min = nodep[argmin];

  for (int i = argmin; i < (*n) - 1; i++) {
    nodep[i] = nodep[i + 1];
  }

  (*n)--;

  return node_min;
}

static Node *build_tree()
{
  int n = 0;
  Node *nodep[nsymbols];

  for (int i = 0; i < nsymbols; i++) {

    if (symbol_count[i] == 0) continue;

    nodep[n] = create_node(i, symbol_count[i], NULL, NULL);
    n++;
  }
  const int dummy = -1;
  while (n >= 2) {
    Node *node1 = pop_min(&n, nodep);
    Node *node2 = pop_min(&n, nodep);

    nodep[n] = create_node(dummy, node1->count + node2->count, node1, node2);
    ++n;
  }

  free(symbol_count);
  return (n==0)?NULL:nodep[0];
}

static void traverse_tree(const int depth, Node *np, const int code, const int tree[], int *number_of_symbols)
{
  if(depth != 0){ //根のノード以外にはこの処理を行います
    for(int i = 0; i < depth-1; ++i){
      if(tree[i] == 1){ //treeの値が1の場合はそこを下に通過する枝があるので、描写します
        printf("  |  ");
      }else if(tree[i] == 0){
        printf("     ");
      }
    }
    printf("  +--");
  }
  
  if(np->symbol == '\n'){  //\nを他の文字と同じように扱うと改行されてしまうので、例外としています
    *number_of_symbols += 1; //ダミーでないシンボルなので加算します
    np->code = code; //符号を10進数で記録します
    np->length = depth; //符号の長さを記録します
    printf("[\\n ]");
    print_binary(code, depth); //符号を表示します
  }else if(np->symbol == ' '){ //スペースも例外とします
    *number_of_symbols += 1;
    np->code = code;
    np->length = depth;
    printf("space");
    print_binary(code, depth);
  }else if(np->symbol != -1){ //その他の文字の場合です
    *number_of_symbols += 1;
    np->code = code;
    np->length = depth;
    printf("[ %c ]", np->symbol);
    print_binary(code, depth);
  }else{ //ダミーノードの場合です
    printf("dummy");
  }
  printf("\n");
  if (np->left == NULL) return; //木の先端までたどり着いたら終了します
  int tree1[30]; //表示を整えるための配列です この場合は深さ29までは対応できます
  int tree2[30];
  for(int i = 0; i < 30; ++i){
    tree1[i] = tree[i];
    tree2[i] = tree[i];
  }
  if(depth != 0){ //根のノード以外ではこの処理を行います
    tree1[depth] = 1; //木の左側に進む場合の配列です
    tree2[depth] = 0; //木の右側に進む場合の配列です
  }else{
    tree1[0] = 1;
    tree2[0] = 0;
  }
  traverse_tree(depth + 1, np->left, code<<1, tree1, number_of_symbols); //木の左側に進む場合です
  traverse_tree(depth + 1, np->right, (1 + (code<<1)), tree2, number_of_symbols); //木の右側に進む場合です
}

int encode(const char *filename1, const char *filename2)
{ //filename1は読み込むファイル、filename2は書き込む先のファイルです
  count_symbols(filename1);
  Node *root = build_tree();
  if (root == NULL){
    fprintf(stderr,"A tree has not been constructed.\n");
    return EXIT_FAILURE;
  }
  
  int tree[30];
  for(int i = 0; i < 30; ++i){
    tree[i] = -1;
  }
  
  int number_of_symbols = 0;
  traverse_tree(0, root, 0, tree, &number_of_symbols);
  
  if(filename2 == NULL){ //filename2がNULLの場合はここで終了です
    return EXIT_SUCCESS;
  }
  
  //ここから先はfilename2に書き込む作業です
  FILE *fp1 = fopen(filename1, "rb");
  FILE *fp2 = fopen(filename2, "ab");
  
  if (fp2 == NULL) {
    fprintf(stderr, "error: cannot open %s\n", filename2);
    exit(1);
  }
  
  fwrite(&number_of_symbols, sizeof(int), 1, fp2); //ファイルにシンボル数を書き込みます
  write_header(root,fp2); //シンボルと符号の対応を書き込みます
  write_body(root, fp1, fp2); //本文を書き込みます
  
  fclose(fp1);
  fclose(fp2);
  return EXIT_SUCCESS;
}
static void print_binary(int n, int depth)
{
  printf("(");
  for(int i = 0; i < depth; ++i){
    if(n & 1 << (depth-1-i)){
      printf("1");
    }else{
      printf("0");
    }
  }
  printf(")");
}

static void node_search(Node *np, Node **ans, const int symbol)
{
  if(np->symbol == symbol){ //一致するシンボルを見つけた場合です
    *ans = np;
    return;
  }else if(np->left != NULL && np->right != NULL){ //シンボルが見つからなかった場合、その左右のノードを探します
    node_search(np->left, ans, symbol);
    node_search(np->right, ans, symbol);
  }
}

static void write_header(const Node *np, FILE *fp)
{
  if(np->symbol != -1){ //ダミー以外のノードの時、書き込む処理を実行します
    fwrite(&(np->symbol), sizeof(int), 1, fp); //シンボルの値を書き込みます
    fwrite(&(np->code), sizeof(int), 1, fp); //符号を10進数表示したものを書き込みます
    fwrite(&(np->length), sizeof(int), 1, fp); //符号の長さを書き込みます
  }else{ //ダミーノードの場合、その左右のノードについて関数を実行します
    write_header(np->left, fp);
    write_header(np->right, fp);
  }
}

static void write_body(Node *root, FILE *fp1, FILE *fp2)
{
  int code_out = 0; //fp2に書き込む値を格納します
  int len_out = 0; //書き込む値を二進数表示した時の(符号に直した時の)長さを格納します これが8になったら一度fp2に書き込み、これとcode_outを0に戻します
  int tmpc;
  Node *tmpcode;
  while((tmpc = fgetc(fp1)) != EOF){ //元のファイルから1文字読み取ります
    node_search(root, &tmpcode, tmpc); //その文字に対応するNode構造体を探します
    for(int i = tmpcode->length -1; i >= 0; --i){ //二進数表示した符号を左から出力用の変数に詰めていきます
      if(tmpcode->code & 1<<i){
        code_out |= 1<<(7-len_out);
      }
      ++len_out;
      if(len_out == 8){ //code_outがchar型1個分の値を格納したら、ファイルに書き込みます
        fwrite(&code_out, sizeof(char), 1, fp2);
        code_out = 0;
        len_out = 0;
      }
    }
  }
  if(len_out != 0){ //最後にファイルに書き込まれていない符号があった場合、書き込みます
    fwrite(&code_out, sizeof(char), 1, fp2);
  }
}

int decode(const char *filename1, const char *filename2)
{ //圧縮されたファイルを解凍する関数です filename1を解凍しfilename2に書き込みます
  FILE *fp1, *fp2;
  if((fp1 = fopen(filename1, "rb")) == NULL){
    fprintf(stderr, "error: cannot open %s\n", filename1);
    return EXIT_FAILURE;
  }
  if((fp2 = fopen(filename2, "w")) == NULL){
    fprintf(stderr, "error: cannot open %s\n", filename2);
    return EXIT_FAILURE;
  }
  int number_of_symbols;
  fread(&number_of_symbols, sizeof(int), 1, fp1); //シンボルの数を読み取ります
  Node *code_list = (Node*)malloc(sizeof(Node) * number_of_symbols); //シンボルの数だけNode構造体を確保します
  for(int i = 0; i < number_of_symbols; ++i){ //シンボルの数だけ、シンボル、符号、符号の長さを読み取るという処理を行います また、読み込んだ値を表示します
    fread(&(code_list[i].symbol), sizeof(int), 1, fp1);
    if(code_list[i].symbol == '\n'){
      printf("\\n:");
    }else if(code_list[i].symbol == ' '){
      printf("space:");
    }else{
      printf("%c:", code_list[i].symbol);
    }
    fread(&(code_list[i].code), sizeof(int), 1, fp1);
    printf("%d,", code_list[i].code);
    fread(&(code_list[i].length), sizeof(int), 1, fp1);
    printf("%d\n", code_list[i].length);
  }
  int tmp_byte;
  int code_out;
  int code_len;
  int char_out;
  while(fread((&tmp_byte), sizeof(char), 1, fp1) == 1){ //1byte分ずつ読み取ります
    for(int i = 7; i >= 0; --i){ //読み取った1byteの数を長さ8の符号として、シンボルに対応している符号がないか探します
      if(tmp_byte & 1<<i){ //符号が1の時、code_outを一つ左シフトし、1を足します
        code_out = (code_out<<1) + 1;
      }else{ //符号が0の時、code_outを一つ左シフトします
        code_out <<= 1;
      }
      ++code_len; //符号が1文字足されたので、符号の長さを1つ大きくします
      if((char_out = search_symbol(code_out, code_len, code_list, number_of_symbols)) != -1){ //長さがcode_len,符号の10進数表示がcode_outのNodeが存在した場合、そのシンボルを書き込みます
        fputc(char_out, fp2);
        code_out = 0;
        code_len = 0;
      }
    }
  }
  free(code_list);
  fclose(fp1);
  fclose(fp2);
  return EXIT_SUCCESS;
}

static int search_symbol(const int code, const int length, const Node *code_list, const int number_of_symbols)
{ //codeとlengthを与え、Nodeの配列から、codeとlengthが一致するノードを探す関数です 一致するノードが存在した場合はそのシンボルを、存在しなかった場合は-1を返します
  for(int i = 0; i < number_of_symbols; ++i){
    if(code_list[i].code == code && code_list[i].length == length){
      return code_list[i].symbol;
    }
  }
  return -1;
}

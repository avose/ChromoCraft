#include "stdio.h"

const char * style = 
    "td { width:5em; height:5em; vertical-align:top; padding: .2em; }\n"
    "div { float: right; width: 1em; height: 1em; margin: 1px; }\n";

void cell(int r, int g, int b) {
  int fg = r+g+b<4 ? 0xffffff : 0x0;
  printf("  <td style='background:rgb(%d,%d,%d); color:#%x'>",
         r*64, g*64, b*64, fg);
  //if (r==3 || g==3 || b==3) 
    printf("<div style='background:rgb(%d,%d,%d)'> </div>", 
	     (r*64)+63, (g*64)+63, (b*64)+63);
  printf("%d,%d,%d", r+1, g+1, b+1);
  printf("</td>\n");
}

void row(int r, int b) {
  int g;
  for (g=0; g<4; ++g) {
    cell(r, g, b);
  }
}

void scan(b) {
  int r;
  for (r=0; r<4; ++r) {
    printf(" <tr>\n");
    row(r, b);
	row(r, b+1);
    printf(" </tr>\n");
  }
}

int main() {
  printf("<html><head><style>\n%s\n"
		 "</style></head><body>\n", style);
  printf("<table>\n");
  scan(0);
  scan(2);
  printf("</table>\n");
  printf("</body></html>");
  return 0;
}

#ifndef ARCA_MOVE_H
#define ARCA_MOVE_H

#include <ctype.h>
#include <stdint.h>
#include <string.h>

#include "util.h"
#include "stone.h"

typedef int coord_t;

#define coord_xy(board, x, y) ((x) + (y) * board_size(board))
#define coord_x(c, b) (board_statics.coord[c][0])
#define coord_y(c, b) (board_statics.coord[c][1])
/* TODO: Smarter way to do this? */
#define coord_dx(c1, c2, b) (coord_x(c1, b) - coord_x(c2, b))
#define coord_dy(c1, c2, b) (coord_y(c1, b) - coord_y(c2, b))

#define pass   -1
#define resign -2
#define is_pass(c)   (c == pass)
#define is_resign(c) (c == resign)

#define coord_is_adjecent(c1, c2, b) (abs(c1 - c2) == 1 || abs(c1 - c2) == board_size(b))
#define coord_is_8adjecent(c1, c2, b) (abs(c1 - c2) == 1 || abs(abs(c1 - c2) - board_size(b)) < 2)

/* 象限:
 * 0 1
 * 2 3
 * (当然是垂直于board_print输出的!)中间坐标包含在低值象限。 */
#define coord_quadrant(c, b) ((coord_x(c, b) > board_size(b) / 2) + 2 * (coord_y(c, b) > board_size(b) / 2))

struct board;
char *coord2bstr(char *buf, coord_t c, struct board *board);
/* 在动态分配的缓冲区中返回坐标字符串。线程安全的. */
char *coord2str(coord_t c, struct board *b);
/* 在静态缓冲区中返回坐标字符串;多个缓冲区被拖放，以启用多个printf()参数，但是除了调试之外，它是不安全的——特别是，它不是线程安全的! */
char *coord2sstr(coord_t c, struct board *b);
coord_t str2coord(char *str, int board_size);


struct move {
	coord_t coord;
	enum stone color;
};


static inline int 
move_cmp(struct move *m1, struct move *m2)
{
	if (m1->color != m2->color)
		return m1->color - m2->color;
	return m1->coord - m2->coord;
}


#endif

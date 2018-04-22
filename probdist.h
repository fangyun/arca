#ifndef ARCA_PROBDIST_H
#define ARCA_PROBDIST_H

/* 根据概率分布来挑选物品的工具 */

/* 概率分布结构设计为一次初始化，然后随机分配一个值，重复随机选取的项。 */

#include "fixp.h"
#include "move.h"
#include "util.h"

struct board;

/* 这个接口看起来有点奇怪，因为我们曾经在不同的“probdist”表示之间切换。 */
struct probdist {
	struct board *b;
	fixp_t *items; // [bsize2], [i] = P(pick==i)
	fixp_t *rowtotals; // [bsize], [i] = sum of items in row i
	fixp_t total; // sum of all items
};


/* 在局部范围内声明pd_对应于棋盘b_. */
#define probdist_alloca(pd_, b_) \
	fixp_t pd_ ## __pdi[board_size2(b_)] __attribute__((aligned(32))); memset(pd_ ## __pdi, 0, sizeof(pd_ ## __pdi)); \
	fixp_t pd_ ## __pdr[board_size(b_)] __attribute__((aligned(32))); memset(pd_ ## __pdr, 0, sizeof(pd_ ## __pdr)); \
	struct probdist pd_ = { .b = b_, .items = pd_ ## __pdi, .rowtotals = pd_ ## __pdr, .total = 0 };

/* 获取给定项的值. */
#define probdist_one(pd, c) ((pd)->items[c])

/* 得到累积的概率值(正态化常数). */
#define probdist_total(pd) ((pd)->total)

/* 设置给定项的值. */
static void probdist_set(struct probdist *pd, coord_t c, fixp_t val);

/* 从总数中删除项目;当您将它传递到一个忽略的列表时，它将被使用到“probdist_pick()”。当然，你必须在事后恢复总数。 */
static void probdist_mute(struct probdist *pd, coord_t c);

/* 选择一个随机项。ignore是一组未被考虑(且其值不在@total)的已排序的项目数组. */
coord_t probdist_pick(struct probdist *pd, coord_t *ignore);


/* 现在，我们做了一些可怕的事情——包括内联helper的board.h */
#include "board.h"


static inline void
probdist_set(struct probdist *pd, coord_t c, fixp_t val)
{
	/* 我们在这里禁用断言，因为这是代码的一个非常关键的部分，而且编译器不愿意在其他函数中嵌入函数。. */
#if 0
	assert(c >= 0 && c < board_size2(pd->b));
	assert(val >= 0);
#endif
	pd->total += val - pd->items[c];
	pd->rowtotals[coord_y(c, pd->b)] += val - pd->items[c];
	pd->items[c] = val;
}

static inline void
probdist_mute(struct probdist *pd, coord_t c)
{
	pd->total -= pd->items[c];
	pd->rowtotals[coord_y(c, pd->b)] -= pd->items[c];
}

#endif

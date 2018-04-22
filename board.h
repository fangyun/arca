/* probdist.h必须包含在board之前，因为我们需要正确的包含顺序. */
#include "probdist.h"

#ifndef ARCA_BOARD_H
#define ARCA_BOARD_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "util.h"
#include "stone.h"
#include "move.h"

struct fbook;


/* 最大支持棋盘大小. (没有S_OFFBOARD边缘.) */
#define BOARD_MAX_SIZE 19


/* board的实现有许多可选的特性.
 * 把它们放在下面: */

#define WANT_BOARD_C // 吃棋串队列

//#define BOARD_SIZE 9 // constant board size, allows better optimization

#define BOARD_PAT3 // 增加3 x3模式代码

//#define BOARD_UNDO_CHECKS 1  // Guard against invalid quick_play() / quick_undo() uses

#define BOARD_MAX_COORDS  ((BOARD_MAX_SIZE+2) * (BOARD_MAX_SIZE+2) )
#define BOARD_MAX_MOVES (BOARD_MAX_SIZE * BOARD_MAX_SIZE)
#define BOARD_MAX_GROUPS (BOARD_MAX_SIZE * BOARD_MAX_SIZE * 2 / 3)
/* For 19x19, max 19*2*6 = 228 groups (stacking b&w stones, each third line empty) */

enum e_sym {
		SYM_FULL,
		SYM_DIAG_UP,
		SYM_DIAG_DOWN,
		SYM_HORIZ,
		SYM_VERT,
		SYM_NONE
};


/* 一些引擎可能正规化他们的读入，跳过对称的动作。我们会告诉他们怎么做。*/
struct board_symmetry {
	/* 下棋在这个长方形里. */
	int x1, x2, y1, y2;
	/* d ==  0: 完整的矩形
	 * d ==  1: 顶部三角形 */
	int d;
	/* 一般的对称类型. */
	/* 请注意，上面的内容是多余的，只是提供了更简单的用法. */
	enum e_sym type;
};


typedef uint64_t hash_t;
#define PRIhash PRIx64

/* XXX: 这确实属于pattern3。不幸的是，这将意味着一个依赖的地狱. */
typedef uint32_t hash3_t; // 3 x3模式散列


/* 请注意，“棋串”只是与我们紧密相连的棋串 */
typedef coord_t group_t;

struct group {
	/* 我们只跟踪到GROUP_KEEP_LIBS;在这一点上，我们不在乎。. */
	/* 这两个值的组合可以在性能调优上有所不同. */
#define GROUP_KEEP_LIBS 10
	// 只有当我们点击它时，才重新填充lib[];这至少是2!
	// Moggy至少需要3个-见下面的语义影响。
#define GROUP_REFILL_LIBS 5
	coord_t lib[GROUP_KEEP_LIBS];
	/* libs是真正气的下限!!! 它只表示lib中的项数，因此您可以依赖它，将真正的气存储到<= GROUP_REFILL_LIBS */
	int libs;
};

struct neighbor_colors {
	char colors[S_MAX];
};


/* 快速攻击，以帮助确保战术代码停留在快速棋局限制。理想情况下，我们有两种不同类型的棋局和快棋局。不过，在这个地方进行强制转换/复制api的想法并没有那么吸引人。*/
#ifndef QUICK_BOARD_CODE
#define FB_ONLY(field)  field
#else
#define FB_ONLY(field)  field ## _disabled
// 试着让错误信息更有帮助 ...
#define clen clen_field_not_supported_for_quick_boards
#define flen flen_field_not_supported_for_quick_boards
#endif

/* 规则集目前几乎没有考虑到;棋局的执行基本上是中国的规则(让子补偿)w/自杀(或者你可以把它看成是新西兰的w/o 让子补偿)，
 * 而引擎则强制不自杀，制定真正的中国规则。然而，我们接受了对手的自杀行为，所以我们应该遵守允许自杀的规则，只是不能充分利用它们。*/
enum go_ruleset {
	RULES_CHINESE, /* 默认值 */
	RULES_AGA,
	RULES_NEW_ZEALAND,
	RULES_JAPANESE,
	RULES_STONES_ONLY, /* 不计数棋眼 */
	/* http://home.snafu.de/jasiek/siming.html */
	/* 简化ING规则-中国规则有让子计数作为点和虚手。也应该允许自杀，但是Arca永远不会自杀。. */
	/* XXX: 我找不到规则文本中虚手的意思，但是Robert Jasiek对它们的解释……这些规则被使用，例如在EGC2012 13x13锦标赛。它们不受KGS的支持。*/
	RULES_SIMING,
};

/* 一个给定大小的所有棋盘共享的数据。 */
struct board_statics {
	int size;	

	/* 为foreach_neighbor*()的迭代器偏移量 */
	int nei8[8], dnei[4];

	/* 坐标的zobrist哈希(黑与白)*/
	hash_t h[BOARD_MAX_COORDS][2];	

	/* 缓存了x-y坐标的信息，避免了分割。 */
	uint8_t coord[BOARD_MAX_COORDS][2];
};

/* 在任何给定的时间内，只使用一块棋盘大小，所以不需要数组。 */
extern struct board_statics board_statics;

/* 您应该将此结构视为只读。如果你想要改变它，总是调用下面的函数。 */

struct board {
	int size; /* 包括S_OFFBOARD边缘 - 见下文. */
	int size2; /* size^2 */
	int bits2; /* ceiling(log2(size2)) */
	int captures[S_MAX];
	floating_t komi;
	int handicap;
	enum go_ruleset rules;
	char *fbookfile;
	struct fbook *fbook;

	int moves;
	struct move last_move;
	struct move last_move2; /* 倒数第二个移动 */
FB_ONLY(struct move last_move3); /* 在last_move2之前，只有当last_move是虚手时才设置。 */
FB_ONLY(struct move last_move4); /* 在last_move3之前，只有当last_move和last_move2是虚手才设置。 */
	/* 我们是否尝试两次添加一个散列;board_play*()可以设置这个，但是它仍然会执行这个动作! */
FB_ONLY(bool superko_violation);

	/* 以下两个结构是goban映射，由coord.pos索引。
	 * 为了加速一些内部循环，映射被从S_OFFBOARD棋子的一个点边缘包围。
	 * 以下的一些foreach迭代器可能包括这些点;如果需要的话，你需要自己处理。 */

	/* 棋盘上有棋子 */
	enum stone b[BOARD_MAX_COORDS];
	/* 棋子是其中一部分的Group id; 0 == no group */
	group_t g[BOARD_MAX_COORDS];
	/* 棋串的下一块棋子的位置; 0 == last stone */
	coord_t p[BOARD_MAX_COORDS];
	/* 邻近的颜色;索引颜色邻居的数量 */
	struct neighbor_colors n[BOARD_MAX_COORDS];

#ifdef BOARD_PAT3
	/* 每个位置的3x3模式码;看到pattern3.h编码规范。这些信息只对空点有效。 */
FB_ONLY(hash3_t pat3)[BOARD_MAX_COORDS];
#endif

	/* 棋串信息-由gid(即基棋串棋子的坐标)索引 */
	struct group gi[BOARD_MAX_COORDS];

	/* 自由位置列表 */
	/* 注意，这里的自由位置是任何有效的着子;包括单点的眼!
	 * 然而，虚手不包括. */
FB_ONLY(coord_t f)[BOARD_MAX_COORDS];  FB_ONLY(int flen);
	/* 为了快速查找，将自由位置坐标映射到它们的列表索引. */
FB_ONLY(int fmap)[BOARD_MAX_COORDS];

#ifdef WANT_BOARD_C
	/* 杀棋串队列 */
FB_ONLY(group_t c)[BOARD_MAX_GROUPS];  FB_ONLY(int clen);
#endif

	/* 对称信息 */
FB_ONLY(struct board_symmetry symmetry);

	/* 在棋盘上的最后一个劫. */
FB_ONLY(struct move last_ko);
FB_ONLY(int last_ko_age);

	/* 基本劫检查 */
	struct move ko;

#ifdef BOARD_UNDO_CHECKS
	/* 防止无效的quick_play() / quick_undo()使用 */
	int quicked;
#endif
	
	/* 引擎特定的状态;持续的通过棋盘的拓展，只在clear_board重置. */
	void *es;

	/* 预演特定的状态; 持续的通过棋盘的拓展,
	 * 由play_random_game()棋盘上初始化; 在摧毁时间free() */
	void *ps;


	/* --- 私有数据 --- */

	/* 为超级劫检查: */

	/* 棋盘“历史” - 哈希碰到的. 哈希的大小应该是 >> board_size^2. */
#define history_hash_bits 12
#define history_hash_mask ((1 << history_hash_bits) - 1)
#define history_hash_prev(i) ((i - 1) & history_hash_mask)
#define history_hash_next(i) ((i + 1) & history_hash_mask)
FB_ONLY(hash_t history_hash)[1 << history_hash_bits];
	/* 当前棋盘位置的哈希. */
FB_ONLY(hash_t hash);
	/* 当前棋盘位置象限的哈希. */
FB_ONLY(hash_t qhash)[4];
};

struct undo_merge {
	group_t	     group;
	coord_t	     last;  
	struct group info;
};

struct undo_enemy {
	group_t      group;
	struct group info;
	coord_t      stones[BOARD_MAX_MOVES];  // TODO try small array
};

struct board_undo {
	struct move last_move2;
	struct move ko;
	struct move last_ko;
	int	    last_ko_age;
	
	coord_t next_at;
	
	coord_t	inserted;
	struct undo_merge merged[4];
	int nmerged;
	int nmerged_tmp;

	struct undo_enemy enemies[4];
	int nenemies;
	int captures; /* number of stones captured */
};


#ifdef BOARD_SIZE
/* 避免未使用的变量的警告 */
#define board_size(b_) (((b_) == (b_)) ? BOARD_SIZE + 2 : 0)
#define board_size2(b_) (board_size(b_) * board_size(b_))
#define real_board_size(b_)  (((b_) == (b_)) ? BOARD_SIZE : 0)
#else
#define board_size(b_) ((b_)->size)
#define board_size2(b_) ((b_)->size2)
#define real_board_size(b_) ((b_)->size - 2)
#endif

/* 这是在小板和大板上采取不同操作的快捷方式(例如，选择不同的变量默认值)。这当然不是最优的，比微调依赖函数值在板上的大小，但是这是困难的，
 * 如果您只对9x9和19x19感兴趣的话，这可能不是非常值得的。 */
#define board_large(b_) (board_size(b_)-2 >= 15)
#define board_small(b_) (board_size(b_)-2 <= 9)

#if BOARD_SIZE == 19
#  define board_bits2(b_) 9
#elif BOARD_SIZE == 13
#  define board_bits2(b_) 8
#elif BOARD_SIZE == 9
#  define board_bits2(b_) 7
#else
#  define board_bits2(b_) ((b_)->bits2)
#endif

#define board_at(b_, c) ((b_)->b[c])
#define board_atxy(b_, x, y) ((b_)->b[(x) + board_size(b_) * (y)])

#define group_at(b_, c) ((b_)->g[c])
#define group_atxy(b_, x, y) ((b_)->g[(x) + board_size(b_) * (y)])

/* 警告!为S_NONE的邻居数没有更新! */
#define neighbor_count_at(b_, coord, color) ((b_)->n[coord].colors[(enum stone) color])
#define set_neighbor_count_at(b_, coord, color, count) (neighbor_count_at(b_, coord, color) = (count))
#define inc_neighbor_count_at(b_, coord, color) (neighbor_count_at(b_, coord, color)++)
#define dec_neighbor_count_at(b_, coord, color) (neighbor_count_at(b_, coord, color)--)
#define immediate_liberty_count(b_, coord) (4 - neighbor_count_at(b_, coord, S_BLACK) - neighbor_count_at(b_, coord, S_WHITE) - neighbor_count_at(b_, coord, S_OFFBOARD))

#define trait_at(b_, coord, color) (b_)->t[coord][(color) - 1]

#define groupnext_at(b_, c) ((b_)->p[c])
#define groupnext_atxy(b_, x, y) ((b_)->p[(x) + board_size(b_) * (y)])

#define group_base(g_) (g_)
#define group_is_onestone(b_, g_) (groupnext_at(b_, group_base(g_)) == 0)
#define board_group_info(b_, g_) ((b_)->gi[(g_)])
#define board_group_captured(b_, g_) (board_group_info(b_, g_).libs == 0)
/* board_group_other_lib()只对具有两个气的棋串有意义。*/
#define board_group_other_lib(b_, g_, l_) (board_group_info(b_, g_).lib[board_group_info(b_, g_).lib[0] != (l_) ? 0 : 1])

#define hash_at(b_, coord, color) (board_statics.h[coord][((color) == S_BLACK ? 1 : 0)])

struct board *board_init(char *fbookfile);
struct board *board_copy(struct board *board2, struct board *board1);
void board_done_noalloc(struct board *board);
void board_done(struct board *board);
/* 这里的size是没有S_OFFBOARD的边界 */
void board_resize(struct board *board, int size);
void board_clear(struct board *board);

typedef void  (*board_cprint)(struct board *b, coord_t c, strbuf_t *buf, void *data);
typedef char *(*board_print_handler)(struct board *b, coord_t c, void *data);
void board_print(struct board *board, FILE *f);
void board_print_custom(struct board *board, FILE *f, board_cprint cprint, void *data);
void board_hprint(struct board *board, FILE *f, board_print_handler handler, void *data);

/* 调试:按字节比较2个棋盘。不要用它来排序=)*/
int board_cmp(struct board *b1, struct board *b2);
/* 同样，但只关心由quick_play() / quick_undo()维护的字段 */
int board_quick_cmp(struct board *b1, struct board *b2);

/* 在棋盘上给出的让子位置;坐标被打印到f. */
void board_handicap(struct board *board, int stones, FILE *f);

/* 返回棋串ID, 0表示允许的自杀，虚手，弃权, -1表示错误 */
int board_play(struct board *board, struct move *m);
/* 像上面一样，但是随机着子;着子坐标被记录到*coord。这种方法永远不会填满你的眼。当没有动作可以进行时，则虚手。
 * 如果你提供自己的许可函数，你可以施加额外的限制;许可函数还可以修改着子坐标，以将着子重定向到其他地方。*/
typedef bool (*ppr_permit)(struct board *b, struct move *m, void *data);
bool board_permit(struct board *b, struct move *m, void *data);
void board_play_random(struct board *b, enum stone color, coord_t *coord, ppr_permit permit, void *permit_data);

/* 撤销，只支持通过着子。返回-1为错误，否则为0。. */
int board_undo(struct board *board);

/* 如果给定着子可以落下，返回true. */
static bool board_is_valid_play(struct board *b, enum stone color, coord_t coord);
static bool board_is_valid_move(struct board *b, struct move *m);
/* 如果劫刚刚被取走，返回true. */
static bool board_playing_ko_threat(struct board *b);
/* 返回0或在打吃的相邻棋串ID. */
static group_t board_get_atari_neighbor(struct board *b, coord_t coord, enum stone group_color);
/* 如果着子没有明显的自我打吃，则返回true. */
static bool board_safe_to_play(struct board *b, coord_t coord, enum stone color);

/* 在一个棋串中确定棋子的数量，最多可以使用@max */
static int group_stone_count(struct board *b, group_t group, int max);

#ifndef QUICK_BOARD_CODE
/* 调整对称信息，好像给定的坐标已经落子. */
void board_symmetry_update(struct board *b, struct board_symmetry *symmetry, coord_t c);
/* 检查坐标是否在对称基底内 (如果是假的，它们可以从基中得到.) */
static bool board_coord_in_symmetry(struct board *b, coord_t c);
#endif

/* 如果给定的坐标有所有给定颜色或边缘的邻居，则返回true. */
static bool board_is_eyelike(struct board *board, coord_t coord, enum stone eye_color);
/* 如果给定坐标可能是假眼，则返回true;只有当你已经知道坐标is_eyelike()时，这个检查才有意义. */
bool board_is_false_eyelike(struct board *board, coord_t coord, enum stone eye_color);
/* 如果给定坐标是1点眼(检查假眼睛，或至少尝试)，返回true. */
bool board_is_one_point_eye(struct board *board, coord_t c, enum stone eye_color);
/* 返回一个1点的眼所有者的棋色，如果不是眼睛的话,返回S_NONE */
enum stone board_get_one_point_eye(struct board *board, coord_t c);

/* board_official_score()是适合于外部表示的评分方法。对于完全填充的棋盘的快速评分(例如:预演)，使用board_fast_score(). */
/* 正: W 赢 */
/* 棋色的比较数+单眼数. */
floating_t board_fast_score(struct board *board);
/* 特洛普-泰勒得分，假设给定棋串实际上已经死亡. */
struct move_queue;
floating_t board_official_score(struct board *board, struct move_queue *mq);
floating_t board_official_score_and_dame(struct board *board, struct move_queue *mq, int *dame);

/* 根据给定的字符串设置棋盘规则。遇到未知的规则集名称，返回false. */
bool board_set_rules(struct board *board, char *name);

/* 快速落子/撤销尝试着子.
 * 警告  只有核心棋盘结构保持 !
 *          两者之间的代码不能依赖其他任何东西.
 *
 * 目前这意味着这些不能被使用:
 *   - 增量模式 (pat3)
 *   - 哈希, superko_violation (spathash, hash, qhash, history_hash)
 *   - 自由位置列表 (f / flen)
 *   - 杀棋串列表 (c / clen)
 *   - 特征 (btraits, t, tq, tqlen)
 *   - last_move3, last_move4, last_ko_age
 *   - 对称信息
 *
 * 在文件的顶部#define QUICK_BOARD_CODE，如果你试图访问一个被禁止的字段，就会出现编译时错误.
 *
 * 如果定义了BOARD_UNDO_CHECKS，无效的quick_play()/quick_undo()组合(例如丢失的撤销)被抓到在下一个board_play().
 */
int  board_quick_play(struct board *board, struct move *m, struct board_undo *u);
void board_quick_undo(struct board *b, struct move *m, struct board_undo *u);

/* quick_play() + quick_undo() 组合.
 * 只有在着子有效时才执行主体。(其他情况默默地忽略).
 * 能在主体中跳出, 但是绝对不要返回/来回跳动!
 * (如果BOARD_UNDO_CHECKS定义，则在运行时捕捉). 虽然可以使用with_move_return(val)返回非嵌套with_move()的值. */
#define with_move(board_, coord_, color_, body_) \
       do { \
	       struct board *board__ = (board_);  /* For with_move_return() */		\
               struct move m_ = { .coord = (coord_), .color = (color_) }; \
               struct board_undo u_; \
               if (board_quick_play(board__, &m_, &u_) >= 0) {	  \
	               do { body_ } while(0);                     \
                       board_quick_undo(board__, &m_, &u_); \
	       }					   \
       } while (0)

/* 在with_move()语句中返回值。只对非嵌套的with_move()有效 */
#define with_move_return(val_)  \
	do {  typeof(val_) val__ = (val_); board_quick_undo(board__, &m_, &u_); return val__;  } while (0)

/* 与with_move()相同，但在无效着子时断言出错. */
#define with_move_strict(board_, coord_, color_, body_) \
       do { \
	       struct board *board__ = (board_);  /* For with_move_return() */		\
               struct move m_ = { .coord = (coord_), .color = (color_) }; \
               struct board_undo u_; \
               assert (board_quick_play(board__, &m_, &u_) >= 0);  \
               do { body_ } while(0);                     \
               board_quick_undo(board__, &m_, &u_); \
       } while (0)


/** 迭代器 */

#define foreach_point(board_) \
	do { \
		coord_t c = 0; \
		for (; c < board_size(board_) * board_size(board_); c++)
#define foreach_point_and_pass(board_) \
	do { \
		coord_t c = pass; \
		for (; c < board_size(board_) * board_size(board_); c++)
#define foreach_point_end \
	} while (0)

#define foreach_free_point(board_) \
	do { \
		int fmax__ = (board_)->flen; \
		for (int f__ = 0; f__ < fmax__; f__++) { \
			coord_t c = (board_)->f[f__];
#define foreach_free_point_end \
		} \
	} while (0)

#define foreach_in_group(board_, group_) \
	do { \
		struct board *board__ = board_; \
		coord_t c = group_base(group_); \
		coord_t c2 = c; c2 = groupnext_at(board__, c2); \
		do {
#define foreach_in_group_end \
			c = c2; c2 = groupnext_at(board__, c2); \
		} while (c != 0); \
	} while (0)

/* 在foreach_point()或另一个foreach_neighbor()中无效，或者更确切地说，在S_OFFBOARD坐标。. */
#define foreach_neighbor(board_, coord_, loop_body) \
	do { \
		struct board *board__ = board_; \
		coord_t coord__ = coord_; \
		coord_t c; \
		c = coord__ - board_size(board__); do { loop_body } while (0); \
		c = coord__ - 1; do { loop_body } while (0); \
		c = coord__ + 1; do { loop_body } while (0); \
		c = coord__ + board_size(board__); do { loop_body } while (0); \
	} while (0)

#define foreach_8neighbor(board_, coord_) \
	do { \
		int fn__i; \
		coord_t c = (coord_); \
		for (fn__i = 0; fn__i < 8; fn__i++) { \
			c += board_statics.nei8[fn__i];
#define foreach_8neighbor_end \
		} \
	} while (0)

#define foreach_diag_neighbor(board_, coord_) \
	do { \
		int fn__i; \
		coord_t c = (coord_); \
		for (fn__i = 0; fn__i < 4; fn__i++) { \
			c += board_statics.dnei[fn__i];
#define foreach_diag_neighbor_end \
		} \
	} while (0)


static inline bool
board_is_eyelike(struct board *board, coord_t coord, enum stone eye_color)
{
	return (neighbor_count_at(board, coord, eye_color)
	        + neighbor_count_at(board, coord, S_OFFBOARD)) == 4;
}

/* 允许棋串自杀 */
static inline bool
board_is_valid_play(struct board *board, enum stone color, coord_t coord)
{
	if (board_at(board, coord) != S_NONE)
		return false;
	if (!board_is_eyelike(board, coord, stone_other(color)))
		return true;
	/* Play within {true,false} eye-ish formation */
	if (board->ko.coord == coord && board->ko.color == color)
		return false;
	int groups_in_atari = 0;
	foreach_neighbor(board, coord, {
		group_t g = group_at(board, c);
		groups_in_atari += (board_group_info(board, g).libs == 1);
	});
	return !!groups_in_atari;
}

/* 检查棋串自杀，慢于board_is_valid_play() */
static inline bool
board_is_valid_play_no_suicide(struct board *board, enum stone color, coord_t coord)
{
	if (board_at(board, coord) != S_NONE)
		return false;
	if (immediate_liberty_count(board, coord) >= 1)
		return true;
	if (board_is_eyelike(board, coord, stone_other(color)) &&
	    board->ko.coord == coord && board->ko.color == color)
			return false;

	//捕获东西?
	foreach_neighbor(board, coord, {
		if (board_at(board, c) == stone_other(color) &&
		    board_group_info(board, group_at(board, c)).libs == 1)
			return true;
	});

	// Neighbour with 2 libs ?
	foreach_neighbor(board, coord, {
		if (board_at(board, c) == color &&
		    board_group_info(board, group_at(board, c)).libs > 1)
			return true;
	});

	return false;  // Suicide
}


static inline bool
board_is_valid_move(struct board *board, struct move *m)
{
	return board_is_valid_play(board, m->color, m->coord);
}

static inline bool
board_playing_ko_threat(struct board *b)
{
	return !is_pass(b->ko.coord);
}

static inline group_t
board_get_atari_neighbor(struct board *b, coord_t coord, enum stone group_color)
{
	foreach_neighbor(b, coord, {
		group_t g = group_at(b, c);
		if (g && board_at(b, c) == group_color && board_group_info(b, g).libs == 1)
			return g;
		/* We return first match. */
	});
	return 0;
}

static inline bool
board_safe_to_play(struct board *b, coord_t coord, enum stone color)
{
	/* 很多自由的邻居 */
	int libs = immediate_liberty_count(b, coord);
	if (libs > 1)
		return true;

	/* 好的，但是我们需要检查一下他们是否有两个libs. */
	coord_t onelib = -1;
	foreach_neighbor(b, coord, {
		if (board_at(b, c) == stone_other(color) && board_group_info(b, group_at(b, c)).libs == 1)
			return true; // can capture; no snapback check

		if (board_at(b, c) != color) continue;
		group_t g = group_at(b, c);
		if (board_group_info(b, g).libs == 1) continue; // in atari
		if (board_group_info(b, g).libs == 2) { // two liberties
			if (libs > 0) return true; // we already have one real liberty
			/* we might be connecting two 2-lib groups, which is ok;
			 * so remember the other liberty and just make sure it's
			 * not the same one */
			if (onelib >= 0 && c != onelib) return true;
			onelib = board_group_other_lib(b, g, c);
			continue;
		}
		// many liberties
		return true;
	});
	// no good support group
	return false;
}

static inline int
group_stone_count(struct board *b, group_t group, int max)
{
	int n = 0;
	foreach_in_group(b, group) {
		n++;
		if (n >= max) return max;
	} foreach_in_group_end;
	return n;
}

#ifndef QUICK_BOARD_CODE
static inline bool
board_coord_in_symmetry(struct board *b, coord_t c)
{
	if (coord_y(c, b) < b->symmetry.y1 || coord_y(c, b) > b->symmetry.y2)
		return false;
	if (coord_x(c, b) < b->symmetry.x1 || coord_x(c, b) > b->symmetry.x2)
		return false;
	if (b->symmetry.d) {
		int x = coord_x(c, b);
		if (b->symmetry.type == SYM_DIAG_DOWN)
			x = board_size(b) - 1 - x;
		if (x > coord_y(c, b))
			return false;
	}
	return true;

}
#endif


#endif

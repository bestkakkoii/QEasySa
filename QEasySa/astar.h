#pragma once
#ifndef __ASTAR_H__
#define __ASTAR_H__
#pragma execution_character_set("utf-8")
#include <QObject>
#include <QPoint>
#include <blockallocator/blockallocator.h>
#include "structs.h"

using Callback = std::function<bool(const QPoint&)>;

class QParam
{
public:
	bool corner;	   // 允許拐角
	int height;		   // 地圖高度
	int width;		   // 地圖寬度
	QPoint start;	   // 起點坐標
	QPoint end;		   // 終點坐標
	Callback can_pass; // 是否可通過

	explicit QParam() : height(0), width(0), corner(true) {}

	explicit QParam(const qmap_t& map, const Callback& callback, const QPoint& start, const QPoint& end) :
		height(map.height), width(map.width), start(start), end(end), corner(true), can_pass(callback)
	{
	}
};

class Map_AStar
{
private:
	/**
	 * 路徑節點狀態
	 */
	typedef enum
	{
		NOTEXIST,               // 不存在
		IN_OPENLIST,            // 在開啟列表
		IN_CLOSEDLIST           // 在關閉列表
	}NodeState;

	/**
	 * 路徑節點
	 */
	struct Node
	{
		int         g;          // 與起點距離
		int         h;          // 與終點距離
		QPoint      pos;        // 節點位置
		NodeState   state;      // 節點狀態
		Node* parent;     // 父節點

		/**
		 * 計算f值
		 */
		inline int __vectorcall f() const { return g + h; }

		inline Node(const QPoint& pos)
			: g(0), h(0), pos(pos), parent(nullptr), state(NodeState::NOTEXIST)
		{}
	};

public:
	explicit Map_AStar(BlockAllocator* a);

	virtual ~Map_AStar();

public:
	/**
	 * 獲取直行估值
	 */
	constexpr int __vectorcall get_step_value() const;

	/**
	 * 獲取拐角估值
	 */
	constexpr int __vectorcall get_oblique_value() const;

	/**
	 * 設置直行估值
	 */
	constexpr void __vectorcall set_step_value(int value);

	/**
	 * 獲取拐角估值
	 */
	constexpr void __vectorcall set_oblique_value(int value);

	/**
	 * 執行尋路操作
	 */
	QVector<QPoint> __vectorcall find(const QParam& param);

private:
	/**
	 * 清理參數
	 */
	void clear();

	/**
	 * 初始化參數
	 */
	void __vectorcall init(const QParam& param);

	/**
	 * 參數是否有效
	 */
	bool __vectorcall is_vlid_params(const QParam& param) const;

private:
	/**
	 * 二叉堆上濾
	 */
	void __vectorcall percolate_up(int& hole);

	/**
	 * 獲取節點索引
	 */
	bool __vectorcall get_node_index(Node*& node, int* index);

	/**
	 * 計算G值
	 */
	__forceinline int __vectorcall calcul_g_value(Node*& parent, const QPoint& current);

	/**
	 * 計算F值
	 */
	__forceinline int __vectorcall calcul_h_value(const QPoint& current, const QPoint& end);

	/**
	 * 節點是否存在於開啟列表
	 */
	__forceinline bool __vectorcall in_open_list(const QPoint& pos, Node*& out_node);

	/**
	 * 節點是否存在於關閉列表
	 */
	__forceinline bool __vectorcall in_closed_list(const QPoint& pos);

	/**
	 * 是否可通過
	 */
	bool __vectorcall can_pass(const QPoint& pos);

	/**
	 * 當前點是否可到達目標點
	 */
	bool __vectorcall can_pass(const QPoint& current, const QPoint& destination, const bool& allow_corner);

	/**
	 * 查找附近可通過的節點
	 */
	void __vectorcall find_can_pass_nodes(const QPoint& current, const bool& allow_corner, QVector<QPoint>* out_lists);

	/**
	 * 處理找到節點的情況
	 */
	void __vectorcall handle_found_node(Node*& current, Node*& destination);

	/**
	 * 處理未找到節點的情況
	 */
	void __vectorcall handle_not_found_node(Node*& current, Node*& destination, const QPoint& end);

private:
	int                     step_val_;
	int                     oblique_val_;
	std::vector<Node*>          mapping_;
	int                     height_;
	int                     width_;
	Callback                can_pass_;
	std::vector<Node*>      open_list_;
	BlockAllocator* allocator_;
};

#endif

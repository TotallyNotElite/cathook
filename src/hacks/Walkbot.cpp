/*
 * Walkbot.cpp
 *
 *  Created on: Jul 23, 2017
 *      Author: nullifiedcat
 */

#include "../common.h"

namespace hacks { namespace shared { namespace walkbot {

using index_t = unsigned;

constexpr index_t INVALID_NODE = unsigned(-1);
constexpr size_t MAX_CONNECTIONS = 4;

enum ENodeFlags {
	NF_GOOD = (1 << 0),
	NF_DUCK = (1 << 1),
	NF_JUMP = (1 << 2)
};

enum EWalkbotState {
	WB_DISABLED,
	WB_RECORDING,
	WB_EDITING,
	WB_REPLAYING
};

struct walkbot_header_s {
	unsigned version { 1 };
	unsigned node_count { 0 };
};

struct walkbot_node_s {
	union {
		struct {
			float x { 0.0f }; // 4
			float y { 0.0f }; // 8
			float z { 0.0f }; // 12
		};
		Vector xyz { 0, 0, 0 };
	};
	unsigned flags { 0 }; // 16
	size_t connection_count { 0 }; // 20
	index_t connections[MAX_CONNECTIONS]; // 36
}; // 36

namespace state {

// A vector containing all loaded nodes, used in both recording and replaying
std::vector<walkbot_node_s> nodes {};

// Target node when replaying, selected node when editing, last node when recording
index_t active_node { INVALID_NODE };

// Node closest to your crosshair when editing
index_t closest_node { INVALID_NODE };

// Global state
EWalkbotState state { WB_DISABLED };

// A little bit too expensive function, finds next free node or creates one if no free slots exist
index_t free_node() {
	for (index_t i = 0; i < nodes.size(); i++) {
		if (not (nodes[i].flags & NF_GOOD))
			return i;
	}

	nodes.emplace_back();
	return nodes.size() - 1;
}

bool node_good(index_t node) {
	return node < nodes.size() && (nodes[node].flags & NF_GOOD);
}

}

CatVar pause_recording(CV_SWITCH, "wb_recording_paused", "0", "Pause recording", "Use BindToggle with this");
CatVar draw_info(CV_SWITCH, "wb_info", "1", "Walkbot info");
CatVar draw_path(CV_SWITCH, "wb_path", "1", "Walkbot path");
CatVar draw_nodes(CV_SWITCH, "wb_nodes", "1", "Walkbot nodes");
CatVar draw_indices(CV_SWITCH, "wb_indices", "1", "Node indices");

void Initialize() {
}

void UpdateClosestNode() {
	float n_fov = 360.0f;
	index_t n_idx = INVALID_NODE;

	for (index_t i = 0; i < state::nodes.size(); i++) {
		const auto& node = state::nodes[i];

		if (not node.flags & NF_GOOD)
			continue;

		float fov = GetFov(g_pLocalPlayer->v_OrigViewangles, g_pLocalPlayer->v_Eye, node.xyz);
		if (fov < n_fov) {
			n_fov = fov;
			n_idx = i;
		}
	}

	// Don't select a node if you don't even look at it
	if (n_fov < 10)
		state::closest_node = n_idx;
	else
		state::closest_node = INVALID_NODE;
}

// Draws a single colored connection between 2 nodes
void DrawConnection(index_t a, index_t b) {
	if (not (state::node_good(a) and state::node_good(b)))
		return;

	const auto& a_ = state::nodes[a];
	const auto& b_ = state::nodes[b];

	Vector wts_a, wts_b;
	if (not (draw::WorldToScreen(a_.xyz, wts_a) and draw::WorldToScreen(b_.xyz, wts_b)))
		return;

	rgba_t* color = &colors::white;
	if 		((a_.flags & b_.flags) & NF_JUMP) color = &colors::yellow;
	else if ((a_.flags & b_.flags) & NF_DUCK) color = &colors::green;

	drawgl::Line(wts_a.x, wts_a.y, wts_b.x - wts_a.x, wts_b.y - wts_a.y, color->rgba);
}

// Draws a node and its connections
void DrawNode(index_t node, bool draw_back) {
	if (not state::node_good(node))
		return;

	const auto& n = state::nodes[node];

	for (size_t i = 0; i < n.connection_count && i < MAX_CONNECTIONS; i++) {
		index_t connection = n.connections[i];
		// To prevent drawing connections twice in a for loop, we only draw connections to nodes with higher index
		if (not draw_back) {
			if (connection < node)
				continue;
		}
		DrawConnection(node, connection);
	}

	if (draw_nodes) {
		rgba_t* color = &colors::white;
		if 		(n.flags & NF_JUMP) color = &colors::yellow;
		else if (n.flags & NF_DUCK) color = &colors::green;

		Vector wts;
		if (not draw::WorldToScreen(n.xyz, wts))
			return;

		size_t node_size = 2;
		if (node == state::closest_node)
			node_size = 4;
		if (node == state::active_node)
			color = &colors::red;

		drawgl::Rect(wts.x - node_size, wts.y - node_size, 2 * node_size, 2 * node_size, color->rgba);
	}

	if (draw_indices) {
		rgba_t* color = &colors::white;
		if 		(n.flags & NF_JUMP) color = &colors::yellow;
		else if (n.flags & NF_DUCK) color = &colors::green;

		Vector wts;
		if (not draw::WorldToScreen(n.xyz, wts))
			return;

		FTGL_Draw(std::to_string(node), wts.x, wts.y, fonts::ftgl_ESP, *color);
	}
}

void DrawPath() {
	for (index_t i = 0; i < state::nodes.size(); i++) {
		DrawNode(i, false);
	}
}

void Draw() {
	if (state::state == WB_DISABLED) return;
	switch (state::state) {
	case WB_RECORDING: {
		AddSideString("Walkbot: Recording");

	} break;
	case WB_EDITING: {
		AddSideString("Walkbot: Editing");

	} break;
	case WB_REPLAYING: {
		AddSideString("Walkbot: Replaying");

	} break;
	}
}

void Move() {
	if (state == WB_DISABLED) return;

}

}}}

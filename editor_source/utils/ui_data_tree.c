#include <fire_ui/fire_ui.h>

#include "ui_data_tree.h"

static const float ARROW_AREA_WIDTH = 25.f;

// For the input pass, we need to iterate through the entire tree... let's just do that for now.
// The last node is returned.
/*static UI_DataTreeNode* UI_DataTreeKeyboardInputPass(UI_DataTree* tree, UI_DataTreeState* state,
	UI_DataTreeNode* parent, UI_DataTreeNode* prev, UI_DataTreeNode* node)
{
	UI_DataTreeNodeState* node_state = tree->GetNodeState(node);
	UI_DataTreeNode* last_node = node;

	if (node_state->is_open) {
		UI_DataTreeNode* prev_child = node;
		for (UI_DataTreeNode* child = tree->GetFirstChild(node); child; child = tree->GetNext(child)) {
			prev_child = UI_DataTreeKeyboardInputPass(tree, state, node, prev_child, child);
			last_node = prev_child;
		}
	}
	
	if (state->selection == node_state->node_key) {
		if (UI_InputWasPressedOrRepeated(UI_Input_Right) && !state->moved_this_frame) {
			if (tree->GetFirstChild(node) && node_state->is_open) {
				state->selection = tree->GetNodeState(tree->GetFirstChild(node))->node_key;
			} else {
				node_state->is_open = true;
			}
			state->moved_this_frame = true;
		}
		if (UI_InputWasPressedOrRepeated(UI_Input_Left) && !state->moved_this_frame) {
			if (tree->GetFirstChild(node) == NULL || !node_state->is_open) {
				if (parent) state->selection = tree->GetNodeState(parent)->node_key;
			} else {
				node_state->is_open = false;
			}
			state->moved_this_frame = true;
		}
	}

	// maybe it'd be easier to just build a flat array of things on the first pass

	if (state->selection == node_state->node_key && UI_InputWasPressedOrRepeated(UI_Input_Up) && !state->moved_this_frame && prev) {
		state->selection = prev;
		state->moved_this_frame = true;
	}
	if (state->selection == prev && UI_InputWasPressedOrRepeated(UI_Input_Down) && !state->moved_this_frame) {
		state->selection = node;
		state->moved_this_frame = true;
	}
	
	return last_node;
}*/

typedef struct UI_DataTreeDrawData {
	int highlight_row_idx;
	int half_highlight_row_idx;
	bool selection_is_alive; // this shouldn't really be here...
} UI_DataTreeDrawData;

typedef struct UI_DataTreeRowDrawData {
	bool has_arrow;
	bool arrow_is_open;
	UI_Font icons_font;
} UI_DataTreeRowDrawData;

static inline UI_Key UI_DataTreeSplittersKey() { return UI_KEY(); }
static inline UI_Key UI_DataTreeDrawDataKey() { return UI_KEY(); }
static inline UI_Key UI_DataTreeRowDrawDataKey() { return UI_KEY(); }

static void UI_DataTreeComputeUnexpandedSize(UI_Box* box, UI_Axis axis, int pass, bool* request_second_pass) {
	// The problem:
	// - We need to know size Y to make splitters
	// - We need to know splitters X to know each element height
	// - We need to know each element height to know size Y
	//
	// The steps for resolving this:
	// 1. put 0 for initial X width
	// 2. Compute all X-sizing using the initial X widths
	// 3. Adjust splitters X sizes
	// 4. Compute all X-sizing again, now with known X-sizes for the node boxes
	// 5. Compute all Y-sizing

	if (axis == UI_Axis_X) {}
	else {
		// Now we're in step 5.
		// Let's do step 3 and 4 first.
		
		UI_SplittersState* splitters;
		UI_BoxGetVar(box, UI_DataTreeSplittersKey(), &splitters);
		UI_SplittersNormalizeToTotalSize(splitters, box->computed_expanded_size.x);
		
		// step 4
		
		float y = 0.f;
		UI_Box* child = box->first_child;
		for (; child ;) {
			
			float x = child->offset.x; // we store indentation temporarily in the first child offset

			float row_height = 0.f;
			for (int column = 0; column < splitters->panel_count; column++) {
				UI_ASSERT(child);

				float column_width = splitters->panel_end_offsets[column] - x;
				child->size[0] = column_width;
				UI_BoxComputeUnexpandedSizeDefault(child, UI_Axis_X, 0, NULL);
				UI_BoxComputeExpandedSize(child, UI_Axis_X, column_width);
				
				child->offset = UI_VEC2{x, y};

				UI_BoxComputeUnexpandedSizeDefault(child, UI_Axis_Y, 0, NULL);
				row_height = UI_Max(row_height, child->computed_unexpanded_size.y);

				x += child->computed_unexpanded_size.x;
				child = child->next;
			}
			y += row_height;
		}

		box->computed_unexpanded_size.y = y;
	}
}

static void UI_DataTreeDraw(UI_Box* box) {
	UI_SplittersState* splitters;
	UI_BoxGetVar(box, UI_DataTreeSplittersKey(), &splitters);
	UI_DataTreeDrawData draw_data;
	UI_BoxGetVar(box, UI_DataTreeDrawDataKey(), &draw_data);
	

	int row_i = 0;
	UI_Box* child = box->first_child;
	while (child) {
		float row_h = 0.f;
		UI_Box* iter = child;
		for (int col = 0; col < splitters->panel_count; col++) {
			row_h = UI_Max(row_h, iter->computed_unexpanded_size.y);
			iter = iter->next;
		}
		
		UI_DataTreeRowDrawData row_draw_data;
		UI_BoxGetVar(child, UI_DataTreeRowDrawDataKey(), &row_draw_data); // store node state / draw data per row in the first box of each row
		
		UI_Rect row_rect = {
			{box->computed_rect.min.x, child->computed_position.y},
			{box->computed_rect.max.x, child->computed_position.y + row_h}
		};

		bool fully_clipped = UI_ClipRect(&row_rect, &box->computed_rect);
		if (!fully_clipped) {
			if ((row_i & 1) == 0) {
				UI_DrawRect(row_rect, UI_COLOR{255, 255, 255, 20});
			}

			if (draw_data.highlight_row_idx == row_i) {
				UI_DrawRectRounded(row_rect, 2.f, UI_COLOR{ 250, 200, 85, 50 }, 5);
				UI_DrawRectLinesRounded(row_rect, 2.f, 2.f, UI_COLOR{ 250, 200, 85, 220 });
			}
			else if (draw_data.half_highlight_row_idx == row_i) {
				UI_DrawRectRounded(row_rect, 2.f, UI_COLOR{0, 0, 0, 210}, 5);
				UI_DrawRectLinesRounded(row_rect, 2.f, 2.f, UI_COLOR{0, 0, 0, 255});
			}

			// draw arrow
			// TODO: I think we should do the arrows as button boxes.
			if (row_draw_data.has_arrow) {
				// TODO: better handling of how data is passed around, especially the icons and icons_font.
				UI_DrawText(row_draw_data.arrow_is_open ? STR_V("\x44") : STR_V("\x46"), row_draw_data.icons_font,
					UI_VEC2{
						child->computed_position.x - ARROW_AREA_WIDTH + UI_DEFAULT_TEXT_PADDING.x,
						child->computed_position.y + UI_DEFAULT_TEXT_PADDING.y
					},
					UI_AlignH_Left, UI_WHITE, &box->computed_rect);
			}
		}
		
		child = iter;
		row_i++;
	}

	UI_DrawBoxDefault(box);
	
	// Draw vertical lines
	for (int i = 1; i < splitters->panel_count; i++) {
		float x = splitters->panel_end_offsets[i - 1];
		UI_Rect line_rect = {
			{box->computed_rect.min.x + x - 1.f, box->computed_rect.min.y},
			{box->computed_rect.min.x + x + 1.f, box->computed_rect.max.y},
		};
		UI_DrawRect(line_rect, UI_BLACK);
	}
}

static void UI_AddDataTreeNode(UI_Key parent_key, UI_Box* root, UI_DataTree* tree, UI_DataTreeDrawData* draw_data,
	UI_DataTreeState* state, UI_SplittersState* splitters, int* row, int indent, UI_DataTreeNode* node)
{
	assert(node->key != 0);
	UI_Key key = UI_HashKey(parent_key, node->key);

	float row_height = 0.f;
	for (int i = 0; i < tree->num_columns; i++) {
		UI_Box* node_box = UI_KBOX(UI_HashInt(UI_KKEY(key), i));
		UI_AddBox(node_box, 0.f, UI_SizeFit(), UI_BoxFlag_Horizontal|UI_BoxFlag_NoAutoOffset);
		node_box->offset.x = ARROW_AREA_WIDTH + (float)indent * ARROW_AREA_WIDTH;

		if (i == 0) { // store node state / draw data per row in the first box of each row
			bool has_arrow_button = node->first_child != NULL;
			if (has_arrow_button && UI_InputWasPressed(UI_Input_MouseLeft) && node_box->prev_frame) {
				UI_Rect arrow_rect = node_box->prev_frame->computed_rect;
				arrow_rect.max.x = node_box->prev_frame->computed_rect.min.x;
				arrow_rect.min.x = arrow_rect.max.x - ARROW_AREA_WIDTH;
				if (UI_IsHoveredIdle(root) && UI_PointIsInRect(arrow_rect, UI_STATE.mouse_pos)) {
					*node->is_open_ptr = !*node->is_open_ptr;
				}
			}
			
			UI_DataTreeRowDrawData row_draw_data;
			row_draw_data.arrow_is_open = *node->is_open_ptr;
			row_draw_data.has_arrow = has_arrow_button;
			row_draw_data.icons_font = tree->icons_font;
			UI_BoxAddVar(node_box, UI_DataTreeRowDrawDataKey(), &row_draw_data);
		}

		UI_PushBox(node_box);
		tree->AddValueUI(tree, node_box, node, *row, i);
		UI_PopBox(node_box);

		if (i == 0) {
			if (tree->allow_selection) {
				if (UI_InputWasPressed(UI_Input_MouseLeft)/* || UI_InputWasPressed(UI_Input_MouseRight)*/) {
					UI_Rect first_node_rect = node_box->prev_frame ? node_box->prev_frame->computed_rect : UI_RECT{0};
					UI_Rect row_rect = {{-10000000.f, first_node_rect.min.y}, {10000000.f, first_node_rect.max.y}};
					if (UI_IsHoveredIdle(root) && UI_PointIsInRect(row_rect, UI_STATE.mouse_pos)) {
						state->pressing = node->key;
					}
				}
			}
			
			if (state->pressing == node->key) {
				//draw_data->half_highlight_row_idx = *row;
			}

			if (state->selection == node->key) draw_data->selection_is_alive = true;

			if (state->pressing != 0) {
				if (state->drag_n_dropping != 0) {
					if (state->selection == node->key) {
						//draw_data->highlight_row_idx = *row;
					}
					if (state->pressing == node->key) {
						draw_data->half_highlight_row_idx = *row;
					}
				}
				else {
					if (state->pressing == node->key) {
						draw_data->highlight_row_idx = *row;
					}
				}
			}
			else {
				if (state->selection == node->key) {
					draw_data->highlight_row_idx = *row;
				}
			}
		}
	}

	*row = *row + 1;

	if (*node->is_open_ptr) {
		for (UI_DataTreeNode* child = node->first_child; child; child = child->next) {
			UI_AddDataTreeNode(key, root, tree, draw_data, state, splitters, row, indent + 1, child);
		}
	}
}

UI_API void UI_AddDataTree(UI_Box* box, UI_Size w, UI_Size h, UI_DataTree* tree, UI_DataTreeState* state) {
	// TODO: make scroll area work here...
	// so we need to put all of this into a box.
	
	// The solution: we can use the prev frame height for first doing the input for UI_Splitters.

	// so lets say that the parent rect has halved during this new frame. That means that whatever "width" values we get from the splitters
	// is totally wrong. so we need a UI_SplittersUpdateAreaRect() function that we deferred call.

	UI_AddBox(box, w, h, 0);

	UI_Rect inputs_rect = box->prev_frame ? box->prev_frame->computed_rect : UI_RECT{0};
	UI_SplittersState* splitters = UI_Splitters(UI_BKEY(box), inputs_rect, UI_Axis_X, tree->num_columns, 6.f);
	
	//for (int i = 1; i < tree->num_columns; i++) {
	//	float offset = splitters->panel_end_offsets[i-1];
	//	UI_Rect line = {{rect.min.x + offset - 1.f, rect.min.y}, {rect.min.x + offset + 1.f, rect.max.y}};
	//	UI_DrawRect(line, UI_BLACK);
	//}

	assert(tree->num_columns >= 1);
	
	UI_DataTreeNode* prev_elem = NULL;
	//state->moved_this_frame = false;

	UI_DataTreeDrawData draw_data = {0};
	draw_data.highlight_row_idx = -1;
	draw_data.half_highlight_row_idx = -1;

	int row_idx = 0;
	UI_PushBox(box);
	for (UI_DataTreeNode* child = tree->root->first_child; child; child = child->next) {
		UI_AddDataTreeNode(UI_BKEY(box), box, tree, &draw_data, state, splitters, &row_idx, 0, child);
	}
	UI_PopBox(box);

	box->compute_unexpanded_size = UI_DataTreeComputeUnexpandedSize;
	box->draw = UI_DataTreeDraw;
	
	// update selection & drag n drop
	{
		// So, the way this should work is this:
		// - We have a "selected" asset. On click (release), the selected asset is set.
		// - We have a "potentially selected" asset which is highlighted as yellow.
		//   But the properties panel only shows the selected asset properties, not the potentially selected asset properties.

		//if (!draw_data.selection_is_alive) {
		//	//state->selection = 0; // clear the selection if the selected row no longer exists
		//}

		state->drag_n_dropping = state->drag_n_dropping_new;
		
		if (UI_InputIsDown(UI_Input_MouseLeft)) {
			state->mouse_travel_after_press.x += UI_Abs(UI_STATE.inputs.mouse_raw_delta.x);
			state->mouse_travel_after_press.y += UI_Abs(UI_STATE.inputs.mouse_raw_delta.y);

			if (tree->allow_drag_n_drop && state->pressing != 0 && state->drag_n_dropping == 0) {
				UI_Vec2 travel = state->mouse_travel_after_press;
				float travel_dist = travel.x*travel.x + travel.y*travel.y;
				if (travel_dist > 500.f) {
					// begin drag n drop
					state->drag_n_dropping_new = state->pressing;
					state->drag_n_dropping = state->pressing;
				}
			}
		}
		else {
			if (state->pressing) {
				if (state->drag_n_dropping == 0) {
					state->selection = state->pressing;
				}
			}

			state->drag_n_dropping_new = 0;
			state->pressing = 0;
			state->mouse_travel_after_press = UI_VEC2{0};
		}
	}

	//if (state->drag_n_dropping) {
	//	//printf("DRAG N DROPPING\n");
	//}

	UI_BoxAddVar(box, UI_DataTreeSplittersKey(), &splitters);
	UI_BoxAddVar(box, UI_DataTreeDrawDataKey(), &draw_data);
}

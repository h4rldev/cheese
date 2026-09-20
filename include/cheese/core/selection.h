#ifndef CHEESE_CORE_SELECTION_H
#define CHEESE_CORE_SELECTION_H

/***********************************/

#include <htils/basictypes.h>
#include <htils/string.h>

#include <cheese/types.h>

/***********************************/

/** @brief Pointer-capture id reserved for a text selection drag.
 * @details Distinct from any semantics id, so the selection pass can tell its
 * own drag apart from a widget capture in @ref cheese_t.active_id.
 */
#define CHEESE_SELECTION_ID (~(u64)0)

//
//
//

/** @brief Record a selectable text run for this frame.
 * @details Called by @ref cheese_draw_text when the style in effect is
 * selectable (@ref cheese_style_get_selectable). Appends the run to the frame's
 * run list, then, if this run is the one the current selection points at, draws
 * the highlight behind the glyphs (so the highlight lags the pointer by one
 * frame). The run list lives in the frame arena and is reset by @ref
 * cheese_begin.
 *
 * @param cheese   The cheese instance.
 * @param text     Borrowed text being drawn (kept for the frame).
 * @param font     Font the run is drawn with.
 * @param x        Pen origin x of the run.
 * @param baseline Baseline y of the run.
 *
 * @pre `cheese` and `text` are non-null and `text` is non-empty.
 *
 * @see cheese_selection_update
 */
void cheese_selection_record(cheese_t *cheese, const string *text,
                             cheese_font_t *font, f32 x, f32 baseline);

//
//
//

/** @brief Run the per-frame selection pass.
 * @details Called by @ref cheese_end after the tree is drawn. Starts a
 * selection when the left button is pressed over a selectable run and no widget
 * captured the pointer, extends it while the selection owns the capture, clears
 * it on a press outside any run, and copies the selection to the clipboard on
 * Ctrl+C while no widget owns the keyboard (@ref cheese_t.edit_id).
 *
 * @param cheese The cheese instance.
 *
 * @see cheese_selection_record
 * @see cheese_selection_copy
 */
void cheese_selection_update(cheese_t *cheese);

//
//
//

/** @brief Drop the current selection.
 * @details Clears the selection state and releases the pointer if the selection
 * owned it.
 *
 * @param cheese The cheese instance.
 */
void cheese_selection_clear(cheese_t *cheese);

//
//
//

/** @brief Whether a non-empty selection exists.
 *
 * @param cheese The cheese instance.
 *
 * @return `true` when a selection spans at least one character.
 */
b32 cheese_selection_active(const cheese_t *cheese);

//
//
//

/** @brief Copy the current selection to the clipboard.
 * @details Copies the selected bytes of the selected run's text through the
 * clipboard seam (@ref cheese_set_clipboard). Does nothing when there is no
 * selection, no clipboard setter, or the run's text changed since the selection
 * started.
 *
 * @param cheese The cheese instance.
 *
 * @return `true` when text was copied.
 */
b32 cheese_selection_copy(cheese_t *cheese);

/***********************************/

#endif // !CHEESE_CORE_SELECTION_H

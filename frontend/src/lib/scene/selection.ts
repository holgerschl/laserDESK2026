/**
 * Shift+click range selection for ordered entity lists (job tree / canvas).
 * Non-shift click replaces selection and sets the anchor for the next range.
 */

export function applyShiftSelection(
	entityCount: number,
	i: number,
	shift: boolean,
	anchorIndex: number | null,
	lastClickedIndex: number | null
): { selectedIndices: number[]; anchorIndex: number | null; lastClickedIndex: number | null } {
	if (i < 0 || i >= entityCount) {
		return {
			selectedIndices: [],
			anchorIndex,
			lastClickedIndex
		};
	}
	if (shift) {
		const a = anchorIndex ?? lastClickedIndex ?? i;
		const lo = Math.max(0, Math.min(a, i));
		const hi = Math.min(entityCount - 1, Math.max(a, i));
		return {
			selectedIndices: Array.from({ length: hi - lo + 1 }, (_, k) => lo + k),
			anchorIndex,
			lastClickedIndex: i
		};
	}
	return {
		selectedIndices: [i],
		anchorIndex: i,
		lastClickedIndex: i
	};
}

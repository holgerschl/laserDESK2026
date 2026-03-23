/** Shared mm tick helpers (DXF preview, scene editor). Drawing units = mm. */

/** ~12 ticks across span; returns step in drawing units (treated as mm in UI). */
export function niceMmStep(span: number): number {
	if (!Number.isFinite(span) || span <= 0) return 1;
	const raw = span / 12;
	const exp = Math.floor(Math.log10(raw));
	const f = raw / Math.pow(10, exp);
	let nf = 1;
	if (f <= 1) nf = 1;
	else if (f <= 2) nf = 2;
	else if (f <= 5) nf = 5;
	else nf = 10;
	return nf * Math.pow(10, exp);
}

export function mmTicks(minV: number, maxV: number, step: number): number[] {
	const ticks: number[] = [];
	const start = Math.ceil((minV - 1e-9) / step) * step;
	let v = start;
	for (let n = 0; n < 400 && v <= maxV + 1e-9; n++, v += step) {
		ticks.push(Number(v.toPrecision(14)));
	}
	return ticks;
}

export function fmtMmLabel(v: number): string {
	const a = Math.abs(v);
	if (a >= 100) return v.toFixed(0);
	if (a >= 10) return v.toFixed(1);
	return v.toFixed(2);
}

/** View layout for SVG with world Y up (flip maps world Y → SVG Y, Y down in viewBox). */
export interface WorldViewLayout {
	minX: number;
	minY: number;
	maxX: number;
	maxY: number;
	w: number;
	h: number;
	flip: (yw: number) => number;
	viewWorldMinY: number;
	viewWorldMaxY: number;
}

/** Fixed stage in mm (0…W × 0…H world), optional symmetric padding for tick margins. */
export function fixedStageLayout(
	stageWidth: number,
	stageHeight: number,
	padMm = 0
): WorldViewLayout {
	const minX = -padMm;
	const maxX = stageWidth + padMm;
	const minY = -padMm;
	const maxY = stageHeight + padMm;
	const w = maxX - minX;
	const h = maxY - minY;
	return {
		minX,
		minY,
		maxX,
		maxY,
		w,
		h,
		flip: (yw: number) => maxY + minY - yw,
		viewWorldMinY: minY,
		viewWorldMaxY: maxY
	};
}

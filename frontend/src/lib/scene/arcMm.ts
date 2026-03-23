/**
 * Circular arc in world mm (+Y up). Angles in degrees: `start_angle_deg` is from +X toward +Y (CCW);
 * `sweep_angle_deg` positive = CCW.
 */

export const DEFAULT_ARC_SWEEP_DEG = 90;

/** First point on the arc (at `start_angle_deg`). */
export function arcFirstPointMm(
	cx: number,
	cy: number,
	r: number,
	startAngleDeg: number
): { x: number; y: number } {
	const rad = (startAngleDeg * Math.PI) / 180;
	return {
		x: cx + r * Math.cos(rad),
		y: cy + r * Math.sin(rad)
	};
}

/** Polyline sampling the arc from start to start+sweep (inclusive endpoints). */
export function tessellateArcMm(
	cx: number,
	cy: number,
	r: number,
	startAngleDeg: number,
	sweepAngleDeg: number,
	segments?: number
): { x: number; y: number }[] {
	if (r <= 0) return [];
	const absSweep = Math.abs(sweepAngleDeg);
	const n =
		segments ?? Math.max(8, Math.min(96, Math.ceil(absSweep / 3) + 1));
	const out: { x: number; y: number }[] = [];
	for (let i = 0; i <= n; i++) {
		const t = i / n;
		const ang = startAngleDeg + sweepAngleDeg * t;
		const rad = (ang * Math.PI) / 180;
		out.push({
			x: cx + r * Math.cos(rad),
			y: cy + r * Math.sin(rad)
		});
	}
	return out;
}

/** Flat array for Konva.Line: [x0,konvaY(y0), ...] */
export function arcToKonvaLinePoints(
	cx: number,
	cy: number,
	r: number,
	startAngleDeg: number,
	sweepAngleDeg: number,
	stageHeight: number,
	konvaY: (wy: number) => number
): number[] {
	const pts = tessellateArcMm(cx, cy, r, startAngleDeg, sweepAngleDeg);
	const flat: number[] = [];
	for (const p of pts) {
		flat.push(p.x, konvaY(p.y));
	}
	return flat;
}

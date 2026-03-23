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

/** End point of the arc (world mm, +Y up). */
export function arcEndPointMm(
	cx: number,
	cy: number,
	r: number,
	startAngleDeg: number,
	sweepAngleDeg: number
): { x: number; y: number } {
	const rad = ((startAngleDeg + sweepAngleDeg) * Math.PI) / 180;
	return {
		x: cx + r * Math.cos(rad),
		y: cy + r * Math.sin(rad)
	};
}

/**
 * Konva.Line flat points in group local space (origin = arc center; Y down).
 * Matches world tessellation with Y flipped for Konva.
 */
export function arcToKonvaLocalLinePoints(
	r: number,
	startAngleDeg: number,
	sweepAngleDeg: number
): number[] {
	const pts = tessellateArcMm(0, 0, r, startAngleDeg, sweepAngleDeg);
	const flat: number[] = [];
	for (const p of pts) {
		flat.push(p.x, -p.y);
	}
	return flat;
}

/**
 * CCW sweep (0, 360] from start to the direction of pointer projected onto the circle.
 */
export function sweepAngleDegCCWToPointer(
	cx: number,
	cy: number,
	_r: number,
	startAngleDeg: number,
	px: number,
	py: number
): number {
	const endDeg = (Math.atan2(py - cy, px - cx) * 180) / Math.PI;
	let d = endDeg - startAngleDeg;
	d %= 360;
	if (d <= 0) d += 360;
	if (d < 1e-4) d = 360;
	return Math.min(360, Math.max(0.25, d));
}

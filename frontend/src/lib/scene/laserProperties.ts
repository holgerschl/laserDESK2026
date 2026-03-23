/** Optional laser / process parameters carried in scene JSON (UI + future RTC list). Backend ignores unknown root keys. */

export interface LaserPropertiesV1 {
	/** Laser power 0–100 % (UI convention). */
	power_percent: number;
	/** Pulse frequency, kHz. */
	frequency_khz: number;
	/** Vector mark speed along segments, mm/s. */
	mark_speed_mm_s: number;
	/** Jump speed between segments, mm/s. */
	jump_speed_mm_s: number;
	/** Optional pulse width, ns (0 = default / not set). */
	pulse_width_ns: number;
}

export const defaultLaserProperties = (): LaserPropertiesV1 => ({
	power_percent: 50,
	frequency_khz: 50,
	mark_speed_mm_s: 250,
	jump_speed_mm_s: 2000,
	pulse_width_ns: 0
});

export function clampLaserProperties(l: LaserPropertiesV1): LaserPropertiesV1 {
	return {
		power_percent: Math.min(100, Math.max(0, l.power_percent)),
		frequency_khz: Math.min(4000, Math.max(0.1, l.frequency_khz)),
		mark_speed_mm_s: Math.min(1e6, Math.max(0.1, l.mark_speed_mm_s)),
		jump_speed_mm_s: Math.min(1e6, Math.max(0.1, l.jump_speed_mm_s)),
		pulse_width_ns: Math.min(1e9, Math.max(0, l.pulse_width_ns))
	};
}

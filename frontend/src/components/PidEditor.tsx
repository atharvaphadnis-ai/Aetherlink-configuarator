import { useState } from "react";

type Props = {
  onApply: (axis: "roll" | "pitch" | "yaw", kp: number, ki: number, kd: number) => Promise<void>;
};

export default function PidEditor({ onApply }: Props) {
  const [axis, setAxis] = useState<"roll" | "pitch" | "yaw">("roll");
  const [kp, setKp] = useState("0.4");
  const [ki, setKi] = useState("0.12");
  const [kd, setKd] = useState("0.015");

  return (
    <div className="rounded-xl border border-slate-800 bg-slate-900 p-3">
      <h3 className="mb-2 text-sm font-semibold text-slate-200">PID Tuning</h3>
      <div className="grid grid-cols-4 gap-2 text-sm">
        <select className="rounded bg-slate-800 p-2 text-slate-100" value={axis} onChange={(e) => setAxis(e.target.value as "roll" | "pitch" | "yaw")}>
          <option value="roll">Roll</option>
          <option value="pitch">Pitch</option>
          <option value="yaw">Yaw</option>
        </select>
        <input className="rounded bg-slate-800 p-2 text-slate-100" value={kp} onChange={(e) => setKp(e.target.value)} />
        <input className="rounded bg-slate-800 p-2 text-slate-100" value={ki} onChange={(e) => setKi(e.target.value)} />
        <input className="rounded bg-slate-800 p-2 text-slate-100" value={kd} onChange={(e) => setKd(e.target.value)} />
      </div>
      <button
        className="mt-3 rounded bg-cyan-600 px-3 py-2 text-sm font-semibold text-white hover:bg-cyan-500"
        onClick={() => onApply(axis, Number(kp), Number(ki), Number(kd))}
      >
        Apply PID
      </button>
    </div>
  );
}

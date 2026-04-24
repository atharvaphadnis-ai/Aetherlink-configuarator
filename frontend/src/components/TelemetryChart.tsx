import { Line, LineChart, ResponsiveContainer, Tooltip, XAxis, YAxis } from "recharts";

type Point = {
  t: number;
  roll: number;
  pitch: number;
  yaw: number;
  bv: number;
};

type Props = {
  points: Point[];
};

export default function TelemetryChart({ points }: Props) {
  return (
    <div className="h-80 w-full rounded-xl border border-slate-800 bg-slate-900 p-3">
      <h3 className="mb-2 text-sm font-semibold text-slate-200">Attitude and Battery</h3>
      <ResponsiveContainer width="100%" height="90%">
        <LineChart data={points}>
          <XAxis dataKey="t" stroke="#94a3b8" />
          <YAxis stroke="#94a3b8" />
          <Tooltip />
          <Line type="monotone" dataKey="roll" stroke="#38bdf8" dot={false} />
          <Line type="monotone" dataKey="pitch" stroke="#22c55e" dot={false} />
          <Line type="monotone" dataKey="yaw" stroke="#f59e0b" dot={false} />
          <Line type="monotone" dataKey="bv" stroke="#e11d48" dot={false} />
        </LineChart>
      </ResponsiveContainer>
    </div>
  );
}

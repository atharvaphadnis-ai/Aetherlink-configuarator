import { useState } from "react";

type Props = {
  logs: string[];
  onSend: (line: string) => Promise<void>;
};

export default function CliPanel({ logs, onSend }: Props) {
  const [line, setLine] = useState("status");

  return (
    <div className="rounded-xl border border-slate-800 bg-slate-900 p-3">
      <h3 className="mb-2 text-sm font-semibold text-slate-200">CLI</h3>
      <div className="mb-2 h-44 overflow-auto rounded bg-slate-950 p-2 text-xs text-slate-300">
        {logs.map((log, i) => (
          <div key={`${log}-${i}`}>{log}</div>
        ))}
      </div>
      <div className="flex gap-2">
        <input
          className="w-full rounded bg-slate-800 p-2 text-sm text-slate-100"
          value={line}
          onChange={(e) => setLine(e.target.value)}
        />
        <button
          className="rounded bg-emerald-600 px-3 py-2 text-sm font-semibold text-white hover:bg-emerald-500"
          onClick={() => onSend(line)}
        >
          Send
        </button>
      </div>
    </div>
  );
}

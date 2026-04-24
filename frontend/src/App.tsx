import { useMemo, useRef, useState } from "react";
import CliPanel from "./components/CliPanel";
import PidEditor from "./components/PidEditor";
import TelemetryChart from "./components/TelemetryChart";
import { SerialClient, type TelemetryFrame } from "./serial";

type TelemetryPoint = { t: number; roll: number; pitch: number; yaw: number; bv: number };

export default function App() {
  const serialRef = useRef<SerialClient | null>(null);
  const [connected, setConnected] = useState(false);
  const [status, setStatus] = useState("Disconnected");
  const [logs, setLogs] = useState<string[]>([]);
  const [telemetry, setTelemetry] = useState<TelemetryPoint[]>([]);
  const [rxProtocol, setRxProtocol] = useState<"sim" | "sbus" | "crsf" | "ibus">("sim");

  const telemetryTail = useMemo(() => telemetry.slice(-600), [telemetry]);

  const appendLog = (line: string) => {
    setLogs((prev) => [...prev.slice(-120), line]);
  };

  const connect = async () => {
    const client = new SerialClient();
    await client.connect(115200);
    serialRef.current = client;
    setConnected(true);
    setStatus("Connected");
    appendLog("Connected to serial device");
    void client.loop((msg) => {
      if (msg.type === "telemetry") {
        const t = msg as unknown as TelemetryFrame & { type: string };
        setTelemetry((prev) => [
          ...prev.slice(-599),
          { t: Math.floor(Date.now() / 1000), roll: t.roll, pitch: t.pitch, yaw: t.yaw, bv: t.bv }
        ]);
      } else {
        appendLog(JSON.stringify(msg));
      }
    });
  };

  const disconnect = async () => {
    await serialRef.current?.disconnect();
    serialRef.current = null;
    setConnected(false);
    setStatus("Disconnected");
    appendLog("Disconnected");
  };

  const send = async (payload: Record<string, unknown>) => {
    if (!serialRef.current) return;
    await serialRef.current.send(payload);
  };

  return (
    <div className="min-h-screen bg-slate-950 p-4 text-slate-100">
      <header className="mb-4 flex items-center justify-between rounded-xl border border-slate-800 bg-slate-900 p-3">
        <div>
          <h1 className="text-lg font-bold">INAV Multi-Board Configurator</h1>
          <p className="text-xs text-slate-400">{status}</p>
        </div>
        <div className="flex gap-2">
          {!connected ? (
            <button className="rounded bg-cyan-600 px-3 py-2 text-sm font-semibold hover:bg-cyan-500" onClick={connect}>
              Connect
            </button>
          ) : (
            <button className="rounded bg-rose-600 px-3 py-2 text-sm font-semibold hover:bg-rose-500" onClick={disconnect}>
              Disconnect
            </button>
          )}
          <button className="rounded bg-amber-600 px-3 py-2 text-sm font-semibold hover:bg-amber-500" onClick={() => send({ cmd: "arm" })}>
            Arm
          </button>
          <button className="rounded bg-slate-700 px-3 py-2 text-sm font-semibold hover:bg-slate-600" onClick={() => send({ cmd: "disarm" })}>
            Disarm
          </button>
        </div>
      </header>

      <main className="grid grid-cols-1 gap-4 lg:grid-cols-2">
        <TelemetryChart points={telemetryTail} />
        <div className="space-y-4">
          <div className="rounded-xl border border-slate-800 bg-slate-900 p-3">
            <h3 className="mb-2 text-sm font-semibold text-slate-200">Receiver and Profiles</h3>
            <div className="flex flex-wrap gap-2">
              <select
                className="rounded bg-slate-800 p-2 text-sm text-slate-100"
                value={rxProtocol}
                onChange={(e) => setRxProtocol(e.target.value as "sim" | "sbus" | "crsf" | "ibus")}
              >
                <option value="sim">SIM</option>
                <option value="sbus">SBUS</option>
                <option value="crsf">CRSF</option>
                <option value="ibus">IBUS</option>
              </select>
              <button className="rounded bg-blue-600 px-3 py-2 text-sm font-semibold hover:bg-blue-500" onClick={() => send({ cmd: "set_rx_protocol", protocol: rxProtocol })}>
                Apply RX
              </button>
              <button className="rounded bg-violet-600 px-3 py-2 text-sm font-semibold hover:bg-violet-500" onClick={() => send({ cmd: "save_config" })}>
                Save Profile
              </button>
              <button className="rounded bg-indigo-600 px-3 py-2 text-sm font-semibold hover:bg-indigo-500" onClick={() => send({ cmd: "load_config" })}>
                Load Profile
              </button>
            </div>
          </div>
          <PidEditor onApply={(axis, kp, ki, kd) => send({ cmd: "set_pid", axis, kp, ki, kd })} />
          <CliPanel logs={logs} onSend={(line) => send({ cmd: "cli", line })} />
        </div>
      </main>
    </div>
  );
}

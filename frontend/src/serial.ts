export type TelemetryFrame = {
  roll: number;
  pitch: number;
  yaw: number;
  gx: number;
  gy: number;
  gz: number;
  bv: number;
  m1: number;
  m2: number;
  m3: number;
  m4: number;
  dt: number;
};

export class SerialClient {
  private port: SerialPort | null = null;
  private reader: ReadableStreamDefaultReader<string> | null = null;
  private writer: WritableStreamDefaultWriter<string> | null = null;
  private decoder = new TextDecoderStream();
  private encoder = new TextEncoderStream();
  private running = false;

  async connect(baudRate = 115200): Promise<void> {
    // @ts-ignore - Web Serial API availability in Chromium only.
    this.port = await navigator.serial.requestPort();
    await this.port.open({ baudRate });
    this.running = true;

    this.port.readable?.pipeTo(this.decoder.writable);
    this.port.writable && this.encoder.readable.pipeTo(this.port.writable);
    this.reader = this.decoder.readable.getReader();
    this.writer = this.encoder.writable.getWriter();
  }

  async disconnect(): Promise<void> {
    this.running = false;
    if (this.reader) await this.reader.cancel();
    this.reader = null;
    if (this.writer) await this.writer.close();
    this.writer = null;
    if (this.port) await this.port.close();
    this.port = null;
  }

  async send(payload: Record<string, unknown>): Promise<void> {
    if (!this.writer) throw new Error("Serial writer not initialized");
    await this.writer.write(`${JSON.stringify(payload)}\n`);
  }

  async loop(onMessage: (msg: Record<string, unknown>) => void): Promise<void> {
    if (!this.reader) return;
    let buffer = "";
    while (this.running) {
      const result = await this.reader.read();
      if (result.done || !result.value) break;
      buffer += result.value;
      const lines = buffer.split("\n");
      buffer = lines.pop() ?? "";
      for (const line of lines) {
        const trimmed = line.trim();
        if (!trimmed) continue;
        try {
          const parsed = JSON.parse(trimmed) as Record<string, unknown>;
          onMessage(parsed);
        } catch {
          // Ignore malformed lines from debug prints.
        }
      }
    }
  }
}

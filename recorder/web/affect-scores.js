export function readAffectScores(stream) {
  if (!stream.selected || stream.isMarker) return null;
  const channels = stream.channels ?? [];
  const indices = ["valence", "arousal"].map(name => channels.findIndex(channel => channel.label?.trim().toLowerCase() === name));
  if (indices.some(index => index < 0)) return null;
  const sample = stream.connected ? stream.preview.at(-1) : undefined;
  return indices.map((index, axis) => ({
    label: axis === 0 ? "Valence" : "Arousal",
    value: Number.isFinite(sample?.values[index]) ? sample.values[index] : null,
    unit: channels[index].unit || "",
  }));
}

export function mountAffectScores(container) {
  let signature = "";
  let fields = [];
  function text(tag, value, className) {
    const node = document.createElement(tag);
    node.textContent = value;
    if (className) node.className = className;
    return node;
  }
  return streams => {
    const scores = streams.map(stream => ({ stream, axes: readAffectScores(stream) })).filter(entry => entry.axes);
    container.hidden = scores.length === 0;
    const nextSignature = JSON.stringify(scores.map(({ stream, axes }) => [stream.id, stream.name, axes.map(axis => axis.unit)]));
    if (signature !== nextSignature) {
      signature = nextSignature;
      fields = [];
      container.replaceChildren();
      for (const { stream, axes } of scores) {
        const group = text("section", "", "affect-scores");
        group.setAttribute("aria-label", `${stream.name} scores`);
        group.title = `${stream.hostname || ""} · ${stream.sourceId || stream.id}`;
        const heading = text("h3", stream.name);
        heading.dataset.fitText = "heading";
        const status = text("p", "Latest received scores", "affect-score-status");
        const values = text("dl", "", "affect-score-values");
        const outputs = axes.map(axis => {
          const cell = text("div", "");
          const label = text("dt", axis.label);
          label.dataset.fitText = "action";
          const description = text("dd", "");
          const value = text("span", "—", "affect-score-number");
          value.dataset.fitText = "heading";
          description.append(value, text("span", axis.unit, "affect-score-unit"));
          cell.append(label, description);
          values.append(cell);
          return value;
        });
        group.append(heading, status, values);
        container.append(group);
        fields.push({ status, outputs });
      }
    }
    scores.forEach(({ stream, axes }, index) => {
      const { status, outputs } = fields[index];
      const message = !stream.connected ? "Disconnected" : axes.every(axis => axis.value === null) ? "Waiting for samples…" : "Latest received scores";
      if (status.textContent !== message) status.textContent = message;
      axes.forEach(({ value }, axis) => {
        const rounded = value === null ? null : Number(value.toFixed(2));
        const formatted = rounded === null ? "—" : `${rounded > 0 ? "+" : ""}${rounded.toFixed(2)}`;
        if (outputs[axis].textContent !== formatted) outputs[axis].textContent = formatted;
      });
    });
  };
}

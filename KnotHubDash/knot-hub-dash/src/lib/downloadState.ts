type Listener = () => void;

interface DlState {
  downloading: boolean;
  downloaded: number;
  total: number | null;
  percent: number;
  taskId: number;
}

let state: DlState = {
  downloading: false,
  downloaded: 0,
  total: null,
  percent: 0,
  taskId: 0,
};

const listeners = new Set<Listener>();

export function getDlState(): DlState {
  return state;
}

export function subscribe(listener: Listener) {
  listeners.add(listener);
  return () => {
    listeners.delete(listener);
  };
}

function notify() {
  listeners.forEach(fn => fn());
}

export function startDownload() {
  state = {
    ...state,
    downloading: true,
    downloaded: 0,
    total: null,
    percent: 0,
    taskId: state.taskId + 1,
  };
  notify();
}

export function updateProgress(downloaded: number, total: number | null) {
  const pct = total ? Math.round((downloaded / total) * 100) : 0;
  state = { ...state, downloaded, total, percent: pct };
  notify();
}

export function finishDownload() {
  state = {
    ...state,
    downloading: false,
    downloaded: 0,
    total: null,
    percent: 0,
  };
  notify();
}

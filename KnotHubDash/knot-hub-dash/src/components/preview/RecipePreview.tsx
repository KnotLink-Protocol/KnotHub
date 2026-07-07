import { useState } from 'react';
import { invoke } from '@tauri-apps/api/core';

interface RecipePreviewProps {
  data: {
    id: string;
    name: string;
    status?: string;
  };
}

export default function RecipePreview({ data }: RecipePreviewProps) {
  const [status, setStatus] = useState(data.status || 'stopped');
  const [content, setContent] = useState('');

  const handleRun = async () => {
    try {
      await invoke('recipe_run', { filePath: data.id });
      setStatus('running');
    } catch (err) {
      alert(`运行失败: ${err}`);
    }
  };

  const handleStop = async () => {
    try {
      await invoke('recipe_stop', { filePath: data.id });
      setStatus('stopped');
    } catch (err) {
      alert(`停止失败: ${err}`);
    }
  };

  const handleView = async () => {
    try {
      const text = await invoke<string>('recipe_read', { filePath: data.id });
      setContent(text);
    } catch (err) {
      alert(`读取失败: ${err}`);
    }
  };

  return (
    <div className="recipe-preview">
      <div className="preview-field">
        <strong>配方名</strong> {data.name}
      </div>
      <div className="preview-field">
        <strong>状态</strong>{' '}
        <span className={status === 'running' ? 'status-badge' : 'status-badge warning'}>
          {status}
        </span>
      </div>
      <div className="preview-field">
        <strong>路径</strong> {data.id}
      </div>
      <hr />
      <div style={{ display: 'flex', gap: 8 }}>
        {status === 'running' ? (
          <button className="btn btn-sm btn-danger" onClick={handleStop}>⏹ 停止</button>
        ) : (
          <button className="btn btn-sm btn-primary" onClick={handleRun}>▶ 运行</button>
        )}
        <button className="btn btn-sm" onClick={handleView}>📄 查看源码</button>
      </div>
      {content && (
        <pre style={{
          marginTop: 12,
          padding: 12,
          background: '#1e1e1e',
          color: '#d4d4d4',
          borderRadius: 6,
          fontSize: 12,
          overflow: 'auto',
        }}>
          {content}
        </pre>
      )}
    </div>
  );
}

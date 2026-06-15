import { useState } from 'react';
import { WebviewWindow } from '@tauri-apps/api/webviewWindow';

export default function Debug() {
  const [activeTool, setActiveTool] = useState<string>('');

  const openFeatureEditor = async () => {
    const win = new WebviewWindow('feature-editor', {
      url: `${window.location.origin}/feature-editor`,
      title: '通用功能清单编辑器',
      width: 1200,
      height: 800,
      resizable: true,
      center: true,
    });
    win.once('tauri://created', () => console.log('编辑器窗口已打开'));
    win.once('tauri://error', (e: unknown) => console.error('窗口创建失败', e));
  };

  return (
    <>
      <div style={{ marginBottom: 16 }}>
        <h1 style={{ fontSize: 22, fontWeight: 500 }}>调试工具</h1>
        <p style={{ color: '#6c757d' }}>开发者工具集</p>
      </div>
      <div className="section">
        <div className="section-header">
          <div className="section-title">工具箱</div>
        </div>
        <div style={{ display: 'flex', gap: 16, flexWrap: 'wrap', marginBottom: 20 }}>
          <button className="tool-btn" onClick={openFeatureEditor}>
            功能清单编辑器
          </button>
          <button
            className={`tool-btn ${activeTool === 'invoker' ? 'active' : ''}`}
            onClick={() => setActiveTool('invoker')}
          >
            功能清单调用器
          </button>
          <button
            className={`tool-btn ${activeTool === 'comm' ? 'active' : ''}`}
            onClick={() => setActiveTool('comm')}
          >
            通讯调试器
          </button>
          <button
            className={`tool-btn ${activeTool === 'docs' ? 'active' : ''}`}
            onClick={() => setActiveTool('docs')}
          >
            开发者文档
          </button>
        </div>
        <div style={{ borderTop: '1px solid #e9ecef', paddingTop: 20 }}>
          {activeTool === 'invoker' && <FeatureInvoker />}
          {activeTool === 'comm' && <CommDebugger />}
          {activeTool === 'docs' && <DeveloperDocs />}
          {!activeTool && <p style={{ color: '#6c757d' }}>点击上方按钮打开对应工具面板</p>}
        </div>
      </div>
    </>
  );
}

// ============== 以下组件保持不变（略作类型修复） ==============

function FeatureInvoker() {
  const [cmd, setCmd] = useState('get_all_nodes');
  const [argsJson, setArgsJson] = useState('');
  const [result, setResult] = useState('等待调用...');

  const handleInvoke = async () => {
    let args = {};
    try {
      if (argsJson.trim()) args = JSON.parse(argsJson);
    } catch(e) {
      setResult(`参数 JSON 解析错误: ${e instanceof Error ? e.message : String(e)}`);
      return;
    }
    setResult('⏳ 正在调用...');
    setTimeout(() => {
      setResult(`✅ 模拟调用成功\n命令: ${cmd}\n参数: ${JSON.stringify(args)}\n返回: { "status": "ok", "data": "示例数据" }`);
    }, 500);
  };

  return (
    <>
      <h4 style={{ marginBottom: 12 }}>调用后端命令</h4>
      <div style={{ display: 'flex', gap: 12, flexWrap: 'wrap', marginBottom: 12 }}>
        <select className="btn" value={cmd} onChange={(e) => setCmd(e.target.value)} style={{ background: 'white' }}>
          <option value="get_all_nodes">get_all_nodes (示例)</option>
          <option value="get_link_status">get_link_status (示例)</option>
          <option value="ping_node">ping_node (需实现)</option>
        </select>
        <input
          type="text"
          placeholder="参数 JSON (可选)"
          value={argsJson}
          onChange={(e) => setArgsJson(e.target.value)}
          style={{ padding: '4px 8px', border: '1px solid #ced4da', borderRadius: 4 }}
        />
        <button className="btn btn-primary" onClick={handleInvoke}>调用</button>
      </div>
      <pre className="invoke-result">{result}</pre>
    </>
  );
}

function CommDebugger() {
  const [eventName, setEventName] = useState('');
  const [eventPayload, setEventPayload] = useState('');
  const [httpUrl, setHttpUrl] = useState('');
  const [logs, setLogs] = useState<string[]>(['[日志] 就绪']);

  const addLog = (msg: string) => {
    setLogs(prev => [...prev, `> ${new Date().toLocaleTimeString()} ${msg}`]);
  };

  const handleEmit = async () => {
    if (!eventName) return addLog('请输入事件名称');
    let payload = {};
    try {
      if (eventPayload.trim()) payload = JSON.parse(eventPayload);
    } catch(e) {
      addLog(`事件数据 JSON 解析失败: ${e instanceof Error ? e.message : String(e)}`);
      return;
    }
    addLog(`发送事件: ${eventName} ${JSON.stringify(payload)}`);
    setTimeout(() => addLog(`✅ 事件已发送 (模拟)`), 100);
  };

  const handleHttpGet = async () => {
    if (!httpUrl) return addLog('请输入 URL');
    addLog(`GET ${httpUrl}`);
    try {
      const res = await fetch(httpUrl);
      const text = await res.text();
      addLog(`✅ 响应 ${res.status}: ${text.slice(0, 100)}`);
    } catch(e: any) {
      addLog(`❌ 错误: ${e.message}`);
    }
  };

  return (
    <>
      <h4 style={{ marginBottom: 12 }}>通讯调试器 (Tauri 事件 / HTTP)</h4>
      <div style={{ display: 'flex', gap: 12, marginBottom: 16 }}>
        <input
          type="text"
          placeholder="事件名称 (如 backend-notify)"
          value={eventName}
          onChange={(e) => setEventName(e.target.value)}
          style={{ flex: 1, padding: '4px 8px', border: '1px solid #ced4da', borderRadius: 4 }}
        />
        <textarea
          placeholder="事件数据 JSON"
          rows={2}
          value={eventPayload}
          onChange={(e) => setEventPayload(e.target.value)}
          style={{ flex: 2, padding: '4px 8px', border: '1px solid #ced4da', borderRadius: 4 }}
        />
        <button className="btn" onClick={handleEmit}>发送事件</button>
      </div>
      <div style={{ display: 'flex', gap: 12, marginBottom: 16 }}>
        <input
          type="text"
          placeholder="HTTP URL (如 https://api.example.com/status)"
          value={httpUrl}
          onChange={(e) => setHttpUrl(e.target.value)}
          style={{ flex: 3, padding: '4px 8px' }}
        />
        <button className="btn" onClick={handleHttpGet}>GET</button>
        <button className="btn" onClick={() => alert('POST 演示')}>POST</button>
      </div>
      <div className="comm-log">
        {logs.map((log, idx) => <div key={idx}>{log}</div>)}
      </div>
    </>
  );
}

function DeveloperDocs() {
  const [content, setContent] = useState('点击链接查看内容（示例）');
  return (
    <>
      <h4 style={{ marginBottom: 12 }}>开发者文档</h4>
      <ul style={{ marginLeft: 20, lineHeight: 1.6 }}>
        <li>
          <a href="#" onClick={(e) => { e.preventDefault(); setContent('<h5>Tauri 官方指南</h5><p>请访问 <a href="https://tauri.app" target="_blank">https://tauri.app</a></p>'); }}>
            Tauri 官方指南
          </a>
        </li>
        <li>
          <a href="#" onClick={(e) => { e.preventDefault(); setContent('<h5>命令列表</h5><ul><li>get_all_nodes</li><li>restart_node(nodeId)</li></ul>'); }}>
            本地 API 文档
          </a>
        </li>
        <li>
          <a href="#" onClick={(e) => { e.preventDefault(); setContent('<h5>功能清单格式</h5><pre>{"features": ["node_monitor"]}</pre>'); }}>
            功能清单说明
          </a>
        </li>
      </ul>
      <div style={{ marginTop: 20, padding: 12, background: '#f8f9fa', borderRadius: 6 }} dangerouslySetInnerHTML={{ __html: content }} />
    </>
  );
}
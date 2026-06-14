import { BrowserRouter, Routes, Route, Link, useLocation } from 'react-router-dom';
import { useEffect, useState } from 'react';
import { ThemeProvider } from './context/ThemeContext';
import { useTheme } from './context/ThemeContext';
import ThemeToggle from './components/ThemeToggle';
import './App.css';

// 页面组件
import Home from './pages/Home';
import Nodes from './pages/Nodes';
import Interconnect from './pages/Interconnect';
import ServiceStatus from './pages/ServiceStatus';
import Debug from './pages/Debug';
import Settings from './pages/Settings';

function App() {
  return (
    <ThemeProvider>
      <BrowserRouter>
        <div className="app">
          <NavBar />
          <MainContent />
          <PreviewBar />
        </div>
      </BrowserRouter>
    </ThemeProvider>
  );
}

function NavBar() {
  const location = useLocation();
  const navItems = [
    { path: '/', label: '主页' },
    { path: '/nodes', label: '节点列表' },
    { path: '/interconnect', label: '互联配方' },
    { path: '/service', label: '服务状态' },
    { path: '/debug', label: '开发工具' },
    { path: '/settings', label: '设置' },
  ];
  return (
    <div className="nav-bar">
      <div className="logo">KnotHub 服务中枢</div>
      <nav>
        {navItems.map(item => (
          <Link
            key={item.path}
            to={item.path}
            className={`nav-item ${location.pathname === item.path ? 'active' : ''}`}
          >
            {item.label}
          </Link>
        ))}
      </nav>
      <div className="footer-note">
        双机热备 · 实时同步
        <ThemeToggle />
      </div>
    </div>
  );
}

function MainContent() {
  return (
    <div className="main-bar">
      <Routes>
        <Route path="/" element={<Home />} />
        <Route path="/nodes" element={<Nodes />} />
        <Route path="/interconnect" element={<Interconnect />} />
        <Route path="/service" element={<ServiceStatus />} />
        <Route path="/debug" element={<Debug />} />
        <Route path="/settings" element={<Settings />} />
      </Routes>
    </div>
  );
}

function PreviewBar() {
  const [previewData, setPreviewData] = useState<{ type: string; id: string; details?: any } | null>(null);

  // 监听来自各页面触发预览更新的事件
  useEffect(() => {
    const handler = (event: CustomEvent) => {
      setPreviewData(event.detail);
    };
    window.addEventListener('update-preview', handler as EventListener);
    return () => window.removeEventListener('update-preview', handler as EventListener);
  }, []);

  return (
    <div className="preview-bar">
      <div className="preview-title">预览面板</div>
      <div className="preview-content">
        {!previewData && <p>点击中间列表中的项目，此处将显示详细信息。</p>}
        {previewData?.type === 'node' && <NodePreview nodeId={previewData.id} />}
        {previewData?.type === 'link' && <LinkPreview linkId={previewData.id} />}
        {previewData?.type === 'port' && <PortPreview port={previewData.id} />}
      </div>
    </div>
  );
}

// 预览子组件
function NodePreview({ nodeId }: { nodeId: string }) {
  const details: Record<string, any> = {
    'node-01': { ip: '10.2.10.101', cpu: '12%', mem: '3.2/8 GB', role: '主节点', lastSeen: '2025-03-21 14:32:05' },
    'node-02': { ip: '10.2.10.102', cpu: '34%', mem: '5.1/8 GB', role: '热备从机', lastSeen: '2025-03-21 14:32:01' },
    'node-03': { ip: '10.2.10.103', cpu: '0%', mem: 'N/A', role: '待命', lastSeen: '2025-03-21 10:15:22' },
  };
  const info = details[nodeId] || { ip: '--', cpu: '--', mem: '--', role: '--', lastSeen: '--' };
  return (
    <>
      <div className="preview-field"><strong>节点ID</strong> {nodeId}</div>
      <div className="preview-field"><strong>IP地址</strong> {info.ip}</div>
      <div className="preview-field"><strong>角色</strong> {info.role}</div>
      <div className="preview-field"><strong>CPU使用</strong> {info.cpu}</div>
      <div className="preview-field"><strong>内存使用</strong> {info.mem}</div>
      <div className="preview-field"><strong>最后心跳</strong> {info.lastSeen}</div>
      <hr />
      <button className="btn btn-sm" onClick={() => alert(`[演示] 重启节点 ${nodeId}`)}>重启节点</button>
      <button className="btn btn-sm" onClick={() => alert(`[演示] 热备切换 ${nodeId}`)}>热备切换</button>
    </>
  );
}

function LinkPreview({ linkId }: { linkId: string }) {
  const details: Record<string, any> = {
    'main-link': { type: '主骨干链路', rate: '12.4 Mbps', latency: '8ms', status: '已连接' },
    'backup-link': { type: '备份链路-电信', rate: '3.2 Mbps', latency: '25ms', status: '波动' },
    'hot-link': { type: '专线热备链路', rate: '0 Mbps', latency: 'N/A', status: '待命' },
  };
  const info = details[linkId] || { type: '未知', rate: '--', latency: '--', status: '--' };
  return (
    <>
      <div className="preview-field"><strong>链路名称</strong> {info.type}</div>
      <div className="preview-field"><strong>实时速率</strong> {info.rate}</div>
      <div className="preview-field"><strong>延迟</strong> {info.latency}</div>
      <div className="preview-field"><strong>状态</strong> {info.status}</div>
      <hr />
      <button className="btn btn-sm" onClick={() => alert(`[演示] 断开 ${linkId}`)}>断开</button>
      <button className="btn btn-sm" onClick={() => alert(`[演示] 重连 ${linkId}`)}>重连</button>
    </>
  );
}

function PortPreview({ port }: { port: string }) {
  return (
    <>
      <div className="preview-field"><strong>端口</strong> {port}</div>
      <button className="btn btn-sm" onClick={() => alert(`重新检查端口 ${port}`)}>重新检查</button>
    </>
  );
}

export default App;
import { BrowserRouter, Routes, Route, Link, useLocation } from 'react-router-dom';
import { useEffect, useState } from 'react';
import { ThemeProvider } from './context/ThemeContext';
import ThemeToggle from './components/ThemeToggle';
import './App.css';

// 页面组件
import Home from './pages/Home';
import Nodes from './pages/Nodes';
import Interconnect from './pages/Interconnect';
import ServiceStatus from './pages/ServiceStatus';
import Debug from './pages/Debug';
import Settings from './pages/Settings';

// 导入拆分后的预览组件
import NodePreview from './components/preview/NodePreview';
import LinkPreview from './components/preview/LinkPreview';
import PortPreview from './components/preview/PortPreview';
import RecipePreview from './components/preview/RecipePreview';


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
        {previewData?.type === 'node' && (
          <NodePreview nodeId={previewData.id} nodeType={previewData.nodeType || 'plugin'} />
        )}
        {previewData?.type === 'link' && <LinkPreview linkId={previewData.id} />}
        {previewData?.type === 'port' && <PortPreview port={previewData.id} />}
        {previewData?.type === 'recipe' && <RecipePreview data={previewData.details} />}
        {/* 后续可以添加 recipe 等 */}
      </div>
    </div>
  );
}

export default App;
import React, { useState, useEffect, JSX } from 'react';
import './FeatureEditor.css';

// 定义数据类型
type ArgType = 'input' | 'optional';

interface ArgInput {
  type: 'input';
  description: string;
  defaultVal: string;
}

interface ArgOptional {
  type: 'optional';
  description: string;
  options: [string, string][]; // [描述, 值]
}

type Arg = ArgInput | ArgOptional;

interface OpenSocketFunc {
  appID: string;
  openSocketID: string;
  description: string;
  args: Record<string, Arg>;
  returns: [string, string][]; // [名称, 描述]
}

interface SignalFunc {
  appID: string;
  signalID: string;
  description: string;
  returns: Record<string, { description: string; verification: string }>;
}

interface AppData {
  appName: string;
  openSocket: Record<string, OpenSocketFunc>;
  signal: Record<string, SignalFunc>;
}

const defaultAppData: AppData = {
  appName: '示例应用',
  openSocket: {},
  signal: {},
};

const STORAGE_KEY = 'feature-manifest';

const FeatureEditor: React.FC = () => {
  const [appData, setAppData] = useState<AppData>(defaultAppData);
  const [currentFunction, setCurrentFunction] = useState<{ type: 'openSocket' | 'signal'; key: string } | null>(null);
  const [importJsonText, setImportJsonText] = useState('');

  // 加载保存的数据（防御性处理）
  useEffect(() => {
    const stored = localStorage.getItem(STORAGE_KEY);
    if (stored) {
      try {
        const data = JSON.parse(stored);
        // 确保数据结构完整
        if (data && typeof data === 'object' && 'openSocket' in data && 'signal' in data) {
          setAppData(data);
        } else {
          console.warn('存储的数据结构无效，使用默认值');
          setAppData(defaultAppData);
        }
      } catch (e) {
        console.error('加载失败', e);
        setAppData(defaultAppData);
      }
    } else {
      setAppData(defaultAppData);
    }
  }, []);

  const saveToLocalStorage = (data: AppData) => {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(data));
  };

  const updateAppData = (newData: AppData) => {
    setAppData(newData);
    saveToLocalStorage(newData);
  };

  const createNewApp = () => {
    const newData = { ...defaultAppData, appName: '新应用' };
    updateAppData(newData);
    setCurrentFunction(null);
  };

  const addOpenSocketFunc = () => {
    const name = prompt('请输入功能名称:');
    if (!name) return;
    if (appData.openSocket[name]) {
      alert('功能名称已存在');
      return;
    }
    const newOpenSocket: OpenSocketFunc = {
      appID: '0x00000000',
      openSocketID: '0x00000000',
      description: '功能描述',
      args: {},
      returns: [],
    };
    const newData = {
      ...appData,
      openSocket: { ...appData.openSocket, [name]: newOpenSocket },
    };
    updateAppData(newData);
    setCurrentFunction({ type: 'openSocket', key: name });
  };

  const addSignalFunc = () => {
    const name = prompt('请输入功能名称:');
    if (!name) return;
    if (appData.signal[name]) {
      alert('功能名称已存在');
      return;
    }
    const newSignal: SignalFunc = {
      appID: '0x00000000',
      signalID: '0x00000000',
      description: '功能描述',
      returns: {},
    };
    const newData = {
      ...appData,
      signal: { ...appData.signal, [name]: newSignal },
    };
    updateAppData(newData);
    setCurrentFunction({ type: 'signal', key: name });
  };

  const deleteFunction = () => {
    if (!currentFunction) return;
    if (!confirm(`确定要删除功能 "${currentFunction.key}" 吗？`)) return;
    const newData = { ...appData };
    if (currentFunction.type === 'openSocket') {
      delete newData.openSocket[currentFunction.key];
    } else {
      delete newData.signal[currentFunction.key];
    }
    updateAppData(newData);
    setCurrentFunction(null);
  };

  const updateAppName = (name: string) => {
    const newData = { ...appData, appName: name };
    updateAppData(newData);
  };

  const exportJson = () => {
    const jsonStr = JSON.stringify(appData, null, 2);
    const blob = new Blob([jsonStr], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `${appData.appName}_功能清单.json`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  };

  const copyJson = async () => {
    const jsonStr = JSON.stringify(appData, null, 2);
    try {
      await navigator.clipboard.writeText(jsonStr);
      alert('JSON已复制到剪贴板');
    } catch (err) {
      console.error('复制失败', err);
    }
  };

  const importJson = () => {
    if (!importJsonText.trim()) {
      alert('请输入JSON数据');
      return;
    }
    try {
      const imported = JSON.parse(importJsonText);
      if (!imported.appName || typeof imported.appName !== 'string') {
        throw new Error('无效的应用数据格式');
      }
      // 确保目标数据结构完整
      const sanitized: AppData = {
        appName: imported.appName,
        openSocket: imported.openSocket || {},
        signal: imported.signal || {},
      };
      updateAppData(sanitized);
      setImportJsonText('');
      setCurrentFunction(null);
      alert('导入成功');
    } catch (error: any) {
      alert('导入失败: ' + error.message);
    }
  };

  const updateOpenSocketFunc = (key: string, updatedFunc: OpenSocketFunc) => {
    const newOpenSocket = { ...appData.openSocket, [key]: updatedFunc };
    const newData = { ...appData, openSocket: newOpenSocket };
    updateAppData(newData);
    setCurrentFunction({ type: 'openSocket', key });
  };

  const updateSignalFunc = (key: string, updatedFunc: SignalFunc) => {
    const newSignal = { ...appData.signal, [key]: updatedFunc };
    const newData = { ...appData, signal: newSignal };
    updateAppData(newData);
    setCurrentFunction({ type: 'signal', key });
  };

  // 渲染功能列表（防御性）
  const renderFunctionList = () => {
    if (!appData) return <div className="empty-state">加载中...</div>;
    const items: JSX.Element[] = [];

    items.push(
      <div
        key="app"
        className={`function-item ${!currentFunction ? 'active' : ''}`}
        onClick={() => setCurrentFunction(null)}
      >
        <div className="function-type">应用</div>
        <h3>{appData.appName}</h3>
        <p>应用基本信息</p>
      </div>
    );

    Object.entries(appData.openSocket || {}).forEach(([key, func]) => {
      const isActive = currentFunction?.type === 'openSocket' && currentFunction.key === key;
      items.push(
        <div
          key={`openSocket-${key}`}
          className={`function-item ${isActive ? 'active' : ''}`}
          onClick={() => setCurrentFunction({ type: 'openSocket', key })}
        >
          <div className="function-type">OpenSocket</div>
          <h3>{key}</h3>
          <p>{func.description}</p>
        </div>
      );
    });

    Object.entries(appData.signal || {}).forEach(([key, func]) => {
      const isActive = currentFunction?.type === 'signal' && currentFunction.key === key;
      items.push(
        <div
          key={`signal-${key}`}
          className={`function-item ${isActive ? 'active' : ''}`}
          onClick={() => setCurrentFunction({ type: 'signal', key })}
        >
          <div className="function-type">Signal</div>
          <h3>{key}</h3>
          <p>{func.description}</p>
        </div>
      );
    });

    if (Object.keys(appData.openSocket || {}).length === 0 && Object.keys(appData.signal || {}).length === 0) {
      items.push(
        <div key="empty" className="empty-state">
          <p>暂无功能，请点击上方按钮添加</p>
        </div>
      );
    }
    return items;
  };

  // 编辑器内容（略作防御）
  const renderEditor = () => {
    if (!currentFunction) {
      return (
        <div>
          <h2>应用基本信息</h2>
          <div className="app-info">
            <div className="form-group">
              <label>应用名称</label>
              <input
                type="text"
                value={appData.appName}
                onChange={(e) => updateAppName(e.target.value)}
              />
            </div>
          </div>
          <div className="action-buttons">
            <button className="btn btn-primary" onClick={() => alert('已保存')}>
              保存
            </button>
          </div>
        </div>
      );
    }

    const { type, key } = currentFunction;
    if (type === 'openSocket') {
      const func = appData.openSocket?.[key];
      if (!func) return <div className="empty-state">功能不存在</div>;
      return (
        <OpenSocketEditor
          func={func}
          funcKey={key}
          onUpdate={(updated) => updateOpenSocketFunc(key, updated)}
          onDelete={deleteFunction}
        />
      );
    } else {
      const func = appData.signal?.[key];
      if (!func) return <div className="empty-state">功能不存在</div>;
      return (
        <SignalEditor
          func={func}
          funcKey={key}
          onUpdate={(updated) => updateSignalFunc(key, updated)}
          onDelete={deleteFunction}
        />
      );
    }
  };

  // OpenSocket 编辑器子组件
  const OpenSocketEditor: React.FC<{
    func: OpenSocketFunc;
    funcKey: string;
    onUpdate: (updated: OpenSocketFunc) => void;
    onDelete: () => void;
  }> = ({ func, funcKey, onUpdate, onDelete }) => {
    const [localFunc, setLocalFunc] = useState(func);

    useEffect(() => {
      setLocalFunc(func);
    }, [func]);

    const handleChange = <K extends keyof OpenSocketFunc>(field: K, value: OpenSocketFunc[K]) => {
      setLocalFunc({ ...localFunc, [field]: value });
    };

    const handleSave = () => {
      onUpdate(localFunc);
      alert('已保存');
    };

    const addArg = () => {
      const name = prompt('请输入参数名称:');
      if (!name) return;
      if (localFunc.args[name]) {
        alert('参数名已存在');
        return;
      }
      const newArgs = {
        ...localFunc.args,
        [name]: { type: 'input' as const, description: '', defaultVal: '' },
      };
      setLocalFunc({ ...localFunc, args: newArgs });
    };

    const updateArg = (argName: string, updates: Partial<Arg>) => {
      const newArgs = { ...localFunc.args };
      newArgs[argName] = { ...newArgs[argName], ...updates } as Arg;
      setLocalFunc({ ...localFunc, args: newArgs });
    };

    const deleteArg = (argName: string) => {
      if (confirm(`确定删除参数 "${argName}" 吗？`)) {
        const newArgs = { ...localFunc.args };
        delete newArgs[argName];
        setLocalFunc({ ...localFunc, args: newArgs });
      }
    };

    const addReturn = () => {
      setLocalFunc({ ...localFunc, returns: [...localFunc.returns, ['', '']] });
    };

    const updateReturn = (index: number, field: 0 | 1, value: string) => {
      const newReturns = [...localFunc.returns];
      newReturns[index][field] = value;
      setLocalFunc({ ...localFunc, returns: newReturns });
    };

    const deleteReturn = (index: number) => {
      if (confirm('确定删除此返回值吗？')) {
        const newReturns = localFunc.returns.filter((_, i) => i !== index);
        setLocalFunc({ ...localFunc, returns: newReturns });
      }
    };

    const changeArgType = (argName: string, newType: ArgType) => {
      const oldArg = localFunc.args[argName];
      if (newType === 'input') {
        updateArg(argName, {
          type: 'input',
          description: oldArg.description,
          defaultVal: (oldArg as any).defaultVal || '',
        });
      } else {
        updateArg(argName, {
          type: 'optional',
          description: oldArg.description,
          options: ((oldArg as any).options || []) as [string, string][],
        });
      }
    };

    const addOption = (argName: string) => {
      const arg = localFunc.args[argName];
      if (arg.type !== 'optional') return;
      const newOptions = [...(arg.options || []), ['', '']] as [string, string][];
      updateArg(argName, { options: newOptions });
    };

    const updateOption = (argName: string, idx: number, field: 0 | 1, value: string) => {
      const arg = localFunc.args[argName];
      if (arg.type !== 'optional') return;
      const newOptions = [...arg.options];
      newOptions[idx][field] = value;
      updateArg(argName, { options: newOptions });
    };

    const deleteOption = (argName: string, idx: number) => {
      if (!confirm('确定删除此选项吗？')) return;
      const arg = localFunc.args[argName];
      if (arg.type !== 'optional') return;
      const newOptions = arg.options.filter((_, i) => i !== idx);
      updateArg(argName, { options: newOptions });
    };

    return (
      <div>
        <h2>编辑 OpenSocket 功能: {funcKey}</h2>
        <div className="app-info">
          <div className="form-group">
            <label>功能名称</label>
            <input type="text" value={funcKey} disabled />
          </div>
          <div className="form-group">
            <label>应用 ID</label>
            <input type="text" value={localFunc.appID} onChange={(e) => handleChange('appID', e.target.value)} />
          </div>
          <div className="form-group">
            <label>OpenSocket ID</label>
            <input type="text" value={localFunc.openSocketID} onChange={(e) => handleChange('openSocketID', e.target.value)} />
          </div>
        </div>
        <div className="form-group">
          <label>功能描述</label>
          <textarea value={localFunc.description} onChange={(e) => handleChange('description', e.target.value)} />
        </div>

        <div className="args-section">
          <h3>参数</h3>
          <div className="action-buttons">
            <button className="btn btn-primary" onClick={addArg}>添加参数</button>
          </div>
          <div className="args-list">
            {Object.keys(localFunc.args).length === 0 && <div className="empty-state">暂无参数</div>}
            {Object.entries(localFunc.args).map(([argName, arg]) => (
              <div key={argName} className="arg-item">
                <div className="arg-header">
                  <h4>{argName}</h4>
                  <button className="btn btn-danger" onClick={() => deleteArg(argName)}>删除</button>
                </div>
                <div className="form-group">
                  <label>类型</label>
                  <select value={arg.type} onChange={(e) => changeArgType(argName, e.target.value as ArgType)}>
                    <option value="input">输入</option>
                    <option value="optional">可选</option>
                  </select>
                </div>
                <div className="form-group">
                  <label>描述</label>
                  <textarea value={arg.description} onChange={(e) => updateArg(argName, { description: e.target.value })} />
                </div>
                {arg.type === 'input' && (
                  <div className="form-group">
                    <label>默认值</label>
                    <input type="text" value={arg.defaultVal} onChange={(e) => updateArg(argName, { defaultVal: e.target.value })} />
                  </div>
                )}
                {arg.type === 'optional' && (
                  <div className="form-group">
                    <label>选项</label>
                    <div className="options-list">
                      {(!arg.options || arg.options.length === 0) && <div className="empty-state">暂无选项</div>}
                      {arg.options.map((opt, idx) => (
                        <div key={idx} className="option-item">
                          <input type="text" value={opt[0]} placeholder="选项描述" onChange={(e) => updateOption(argName, idx, 0, e.target.value)} />
                          <input type="text" value={opt[1]} placeholder="选项值" onChange={(e) => updateOption(argName, idx, 1, e.target.value)} />
                          <button className="btn btn-danger btn-sm" onClick={() => deleteOption(argName, idx)}>删除</button>
                        </div>
                      ))}
                    </div>
                    <div className="action-buttons">
                      <button className="btn btn-primary" onClick={() => addOption(argName)}>添加选项</button>
                    </div>
                  </div>
                )}
              </div>
            ))}
          </div>
        </div>

        <div className="returns-section">
          <h3>返回值</h3>
          <div className="action-buttons">
            <button className="btn btn-primary" onClick={addReturn}>添加返回值</button>
          </div>
          <div className="returns-list">
            {localFunc.returns.length === 0 && <div className="empty-state">暂无返回值</div>}
            {localFunc.returns.map((ret, idx) => (
              <div key={idx} className="return-item">
                <div className="return-header">
                  <h4>返回值 {idx + 1}</h4>
                  <button className="btn btn-danger" onClick={() => deleteReturn(idx)}>删除</button>
                </div>
                <div className="form-group">
                  <label>名称</label>
                  <input type="text" value={ret[0]} onChange={(e) => updateReturn(idx, 0, e.target.value)} />
                </div>
                <div className="form-group">
                  <label>描述</label>
                  <textarea value={ret[1]} onChange={(e) => updateReturn(idx, 1, e.target.value)} />
                </div>
              </div>
            ))}
          </div>
        </div>

        <div className="action-buttons">
          <button className="btn btn-primary" onClick={handleSave}>保存</button>
          <button className="btn btn-danger" onClick={onDelete}>删除功能</button>
        </div>
      </div>
    );
  };

  // Signal 编辑器子组件
  const SignalEditor: React.FC<{
    func: SignalFunc;
    funcKey: string;
    onUpdate: (updated: SignalFunc) => void;
    onDelete: () => void;
  }> = ({ func, funcKey, onUpdate, onDelete }) => {
    const [localFunc, setLocalFunc] = useState(func);

    useEffect(() => {
      setLocalFunc(func);
    }, [func]);

    const handleChange = <K extends keyof SignalFunc>(field: K, value: SignalFunc[K]) => {
      setLocalFunc({ ...localFunc, [field]: value });
    };

    const handleSave = () => {
      onUpdate(localFunc);
      alert('已保存');
    };

    const addReturn = () => {
      const name = prompt('请输入返回值名称:');
      if (!name) return;
      if (localFunc.returns[name]) {
        alert('返回值名称已存在');
        return;
      }
      const newReturns = { ...localFunc.returns, [name]: { description: '', verification: '' } };
      setLocalFunc({ ...localFunc, returns: newReturns });
    };

    const updateReturn = (returnName: string, field: 'description' | 'verification', value: string) => {
      const newReturns = { ...localFunc.returns };
      newReturns[returnName][field] = value;
      setLocalFunc({ ...localFunc, returns: newReturns });
    };

    const deleteReturn = (returnName: string) => {
      if (confirm(`确定删除返回值 "${returnName}" 吗？`)) {
        const newReturns = { ...localFunc.returns };
        delete newReturns[returnName];
        setLocalFunc({ ...localFunc, returns: newReturns });
      }
    };

    return (
      <div>
        <h2>编辑 Signal 功能: {funcKey}</h2>
        <div className="app-info">
          <div className="form-group">
            <label>功能名称</label>
            <input type="text" value={funcKey} disabled />
          </div>
          <div className="form-group">
            <label>应用 ID</label>
            <input type="text" value={localFunc.appID} onChange={(e) => handleChange('appID', e.target.value)} />
          </div>
          <div className="form-group">
            <label>Signal ID</label>
            <input type="text" value={localFunc.signalID} onChange={(e) => handleChange('signalID', e.target.value)} />
          </div>
        </div>
        <div className="form-group">
          <label>功能描述</label>
          <textarea value={localFunc.description} onChange={(e) => handleChange('description', e.target.value)} />
        </div>

        <div className="returns-section">
          <h3>返回值</h3>
          <div className="action-buttons">
            <button className="btn btn-primary" onClick={addReturn}>添加返回值</button>
          </div>
          <div className="returns-list">
            {Object.keys(localFunc.returns).length === 0 && <div className="empty-state">暂无返回值</div>}
            {Object.entries(localFunc.returns).map(([retName, ret]) => (
              <div key={retName} className="return-item">
                <div className="return-header">
                  <h4>{retName}</h4>
                  <button className="btn btn-danger" onClick={() => deleteReturn(retName)}>删除</button>
                </div>
                <div className="form-group">
                  <label>描述</label>
                  <textarea value={ret.description} onChange={(e) => updateReturn(retName, 'description', e.target.value)} />
                </div>
                <div className="form-group">
                  <label>验证</label>
                  <input type="text" value={ret.verification} onChange={(e) => updateReturn(retName, 'verification', e.target.value)} />
                </div>
              </div>
            ))}
          </div>
        </div>

        <div className="action-buttons">
          <button className="btn btn-primary" onClick={handleSave}>保存</button>
          <button className="btn btn-danger" onClick={onDelete}>删除功能</button>
        </div>
      </div>
    );
  };

  const jsonPreview = JSON.stringify(appData, null, 2);

  return (
    <div className="feature-editor-container">
      <header>
        <h1>通用功能清单编辑器</h1>
        <p>创建和编辑功能清单JSON文件</p>
      </header>
      <div className="editor-grid">
        <div className="panel">
          <div className="action-buttons">
            <button className="btn btn-primary" onClick={createNewApp}>新建应用</button>
            <button className="btn btn-primary" onClick={addOpenSocketFunc}>添加OpenSocket功能</button>
            <button className="btn btn-primary" onClick={addSignalFunc}>添加Signal功能</button>
            <button className="btn btn-success" onClick={exportJson}>导出JSON</button>
          </div>
          <div className="function-list">
            {renderFunctionList()}
          </div>
        </div>

        <div className="panel">
          {renderEditor()}
        </div>

        <div className="panel">
          <div className="preview-actions">
            <h3>JSON预览</h3>
            <button className="btn btn-primary" onClick={copyJson}>复制JSON</button>
          </div>
          <pre className="json-preview">{jsonPreview}</pre>
          <div className="import-section">
            <h3>导入JSON</h3>
            <div className="form-group">
              <textarea
                placeholder="粘贴JSON数据到这里..."
                value={importJsonText}
                onChange={(e) => setImportJsonText(e.target.value)}
              />
            </div>
            <div className="action-buttons">
              <button className="btn btn-warning" onClick={importJson}>导入JSON</button>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};

export default FeatureEditor;
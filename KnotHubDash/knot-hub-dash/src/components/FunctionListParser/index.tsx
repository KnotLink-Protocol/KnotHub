import React, { useState } from 'react';
import type { FunctionManifest, OpenSocketFunc } from './types';
import styles from './FunctionListParser.module.css';

interface FunctionListParserProps {
  manifest: FunctionManifest;
  onInvoke: (func: OpenSocketFunc, args: Record<string, string>) => Promise<void>;
}

const FunctionListParser: React.FC<FunctionListParserProps> = ({
  manifest,
  onInvoke,
}) => {
  const [selectedFuncName, setSelectedFuncName] = useState<string | null>(null);
  const [inputValues, setInputValues] = useState<Record<string, string>>({});
  const [loading, setLoading] = useState(false);

  const funcNames = Object.keys(manifest.openSocket);
  const selectedFunc = selectedFuncName ? manifest.openSocket[selectedFuncName] : null;

  // 默认选中第一个功能（如果有）
  React.useEffect(() => {
    if (funcNames.length > 0 && !selectedFuncName) {
      setSelectedFuncName(funcNames[0]);
    }
  }, [funcNames, selectedFuncName]);

  const handleArgChange = (argName: string, value: string) => {
    setInputValues(prev => ({ ...prev, [argName]: value }));
  };

  const buildCallArgs = (func: OpenSocketFunc): Record<string, string> => {
    const result: Record<string, string> = {};
    for (const [argName, arg] of Object.entries(func.args)) {
      if (arg.type === 'static') {
        result[argName] = arg.value;
      } else if (arg.type === 'optional') {
        const selected = inputValues[argName];
        if (selected) {
          result[argName] = selected;
        } else if (arg.options.length > 0) {
          result[argName] = arg.options[0][1];
        }
      } else if (arg.type === 'input') {
        result[argName] = inputValues[argName] ?? arg.defaultVal ?? '';
      }
    }
    return result;
  };

  const handleCall = async () => {
    if (!selectedFunc) return;
    const args = buildCallArgs(selectedFunc);
    setLoading(true);
    try {
      await onInvoke(selectedFunc, args);
    } catch (err) {
      console.error(err);
    } finally {
      setLoading(false);
    }
  };

  const renderArg = (argName: string, arg: any) => {
    if (arg.type === 'static') {
      return (
        <div className={styles.argItem} key={argName}>
          <label className={styles.argLabel}>
            {argName}
            {arg.description && <span className={styles.argDesc}>（{arg.description}）</span>}
          </label>
          <div className={styles.argControl}>
            <span className={styles.staticValue}>{arg.value}</span>
          </div>
        </div>
      );
    } else if (arg.type === 'optional') {
      return (
        <div className={styles.argItem} key={argName}>
          <label className={styles.argLabel}>
            {argName}
            {arg.description && <span className={styles.argDesc}>（{arg.description}）</span>}
          </label>
          <div className={styles.argControl}>
            <select
              value={inputValues[argName] || arg.options[0]?.[1] || ''}
              onChange={(e) => handleArgChange(argName, e.target.value)}
            >
              {arg.options.map(([label, value]: [string, string]) => (
                <option key={value} value={value}>{label}</option>
              ))}
            </select>
          </div>
        </div>
      );
    } else if (arg.type === 'input') {
      return (
        <div className={styles.argItem} key={argName}>
          <label className={styles.argLabel}>
            {argName}
            {arg.description && <span className={styles.argDesc}>（{arg.description}）</span>}
          </label>
          <div className={styles.argControl}>
            <input
              type="text"
              value={inputValues[argName] ?? arg.defaultVal ?? ''}
              placeholder={arg.defaultVal || '请输入'}
              onChange={(e) => handleArgChange(argName, e.target.value)}
            />
          </div>
        </div>
      );
    }
    return null;
  };

  if (funcNames.length === 0) {
    return <div className={styles.noFunc}>该节点暂无可用功能</div>;
  }

  return (
    <div className={styles.parserContainer}>
      {/* 上方：功能列表（横向排列） */}
      <div className={styles.funcList}>
        {funcNames.map(name => (
          <button
            key={name}
            className={`${styles.funcChip} ${selectedFuncName === name ? styles.active : ''}`}
            onClick={() => {
              setSelectedFuncName(name);
              setInputValues({});
            }}
          >
            {name}
          </button>
        ))}
      </div>

      {/* 下方：选中功能的详情 */}
      {selectedFunc && (
        <div className={styles.funcDetail}>
          {/* 描述行 */}
          {selectedFunc.description && (
            <div className={styles.funcDescription}>{selectedFunc.description}</div>
          )}
          <div className={styles.argsForm}>
            {Object.entries(selectedFunc.args).map(([name, arg]) => renderArg(name, arg))}
          </div>
          <button className={styles.callBtn} onClick={handleCall} disabled={loading}>
            {loading ? '调用中...' : '调用'}
          </button>
          {selectedFunc.returns.length > 0 && (
            <div className={styles.returnInfo}>
              <h5>返回值</h5>
              <ul>
                {selectedFunc.returns.map(([name, desc]) => (
                  <li key={name}>{name}：{desc || '无描述'}</li>
                ))}
              </ul>
            </div>
          )}
        </div>
      )}
    </div>
  );
};

export default FunctionListParser;
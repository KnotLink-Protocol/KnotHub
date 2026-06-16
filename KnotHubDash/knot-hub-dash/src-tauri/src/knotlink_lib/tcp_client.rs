use anyhow::Result;
use bytes::BytesMut;
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::TcpStream;
use tokio::sync::mpsc;
use tokio::time::{self, Duration};
use log::{debug, error, info};

pub struct TcpClient {
    stream: TcpStream,
    rx: mpsc::UnboundedReceiver<Vec<u8>>,
    heartbeat_interval: Duration,
}

impl TcpClient {
    pub async fn connect(addr: &str, heartbeat_secs: u64) -> Result<(Self, mpsc::UnboundedSender<Vec<u8>>)> {
        let stream = TcpStream::connect(addr).await?;
        let (tx, rx) = mpsc::unbounded_channel();
        let client = TcpClient {
            stream,
            rx,
            heartbeat_interval: Duration::from_secs(heartbeat_secs),
        };
        Ok((client, tx))
    }

    pub async fn run(self, mut on_data: impl FnMut(Vec<u8>) + Send + 'static) -> Result<()> {
        let TcpClient { mut stream, mut rx, heartbeat_interval } = self;
        let mut heartbeat_timer = time::interval(heartbeat_interval);
        let mut read_buf = BytesMut::with_capacity(1024);

        loop {
            tokio::select! {
                Some(data) = rx.recv() => {
                    if let Err(e) = stream.write_all(&data).await {
                        error!("发送数据失败: {}", e);
                        return Err(e.into());
                    }
                    debug!("发送 {} 字节", data.len());
                }
                _ = heartbeat_timer.tick() => {
                    if let Err(e) = stream.write_all(b"heartbeat").await {
                        error!("发送心跳失败: {}", e);
                        return Err(e.into());
                    }
                    debug!("发送心跳");
                }
                result = stream.read_buf(&mut read_buf) => {
                    match result {
                        Ok(0) => {
                            info!("服务器关闭连接");
                            return Ok(());
                        }
                        Ok(n) => {
                            debug!("收到 {} 字节", n);
                            let data = read_buf.split_to(n).to_vec();
                            let data_str = String::from_utf8_lossy(&data);
                            
if data_str.contains("heartbeat_response") {
    // 移除所有 "heartbeat_response" 子串
    let cleaned = data_str.replace("heartbeat_response", "");
    debug!("过滤心跳响应后剩余: {}", cleaned);
    if !cleaned.is_empty() {
        on_data(cleaned.as_bytes().to_vec());
    }
} else {
    on_data(data);
}
                        }
                        Err(e) => {
                            error!("读取数据失败: {}", e);
                            return Err(e.into());
                        }
                    }
                }
            }
        }
    }
}
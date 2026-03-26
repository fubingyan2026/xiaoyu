# FreeRTOS stream_buffer.c 文件深度分析

## 概述

`stream_buffer.c` 是 FreeRTOS 中实现**流缓冲区（Stream Buffer）**功能的核心文件。它提供了一种高效的数据流传输机制，特别适用于串口通信、网络数据包处理等需要处理连续数据流的场景。

## 流缓冲区 vs 消息队列

### 主要区别

| 特性 | 流缓冲区 | 消息队列 |
|------|----------|----------|
| **数据格式** | 字节流（连续数据） | 离散消息（固定大小） |
| **读取方式** | 任意字节数读取 | 整条消息读取 |
| **内存效率** | 更高（无消息边界开销） | 较低（每条消息有边界） |
| **适用场景** | 串口通信、网络流 | 命令传输、事件通知 |

## 核心数据结构

### 流缓冲区结构体

```c
typedef struct StreamBufferDef_t {
    volatile size_t xTail;                 // 读指针（下一个读取位置）
    volatile size_t xHead;                 // 写指针（下一个写入位置）
    size_t xLength;                        // 缓冲区总长度
    size_t xTriggerLevelBytes;             // 触发级别（唤醒接收任务的字节数）
    volatile TaskHandle_t xTaskWaitingToReceive; // 等待接收的任务句柄
    volatile TaskHandle_t xTaskWaitingToSend;    // 等待发送的任务句柄
    uint8_t *pucBuffer;                    // 缓冲区数据指针
    uint8_t ucFlags;                       // 标志位
} StreamBuffer_t;
```

### 关键宏定义

```c
// 消息长度存储大小
#define sbBYTES_TO_STORE_MESSAGE_LENGTH ( sizeof( configMESSAGE_BUFFER_LENGTH_TYPE ) )

// 标志位定义
#define sbFLAGS_IS_MESSAGE_BUFFER        ( ( uint8_t ) 1 )  // 设置为消息缓冲区
#define sbFLAGS_IS_STATICALLY_ALLOCATED  ( ( uint8_t ) 2 )  // 静态分配内存
```

## 核心功能实现

### 1. 流缓冲区创建

```c
StreamBufferHandle_t xStreamBufferCreate( size_t xBufferSizeBytes,
                                         size_t xTriggerLevelBytes )
{
    StreamBuffer_t *pxStreamBuffer;
    uint8_t *pucBuffer;
    
    // 分配缓冲区内存
    pucBuffer = ( uint8_t * ) pvPortMalloc( xBufferSizeBytes );
    
    // 分配流缓冲区结构体
    pxStreamBuffer = ( StreamBuffer_t * ) pvPortMalloc( sizeof( StreamBuffer_t ) );
    
    // 初始化流缓冲区
    if( ( pucBuffer != NULL ) && ( pxStreamBuffer != NULL ) )
    {
        pxStreamBuffer->xLength = xBufferSizeBytes;
        pxStreamBuffer->pucBuffer = pucBuffer;
        pxStreamBuffer->xHead = 0;
        pxStreamBuffer->xTail = 0;
        pxStreamBuffer->xTriggerLevelBytes = xTriggerLevelBytes;
        pxStreamBuffer->xTaskWaitingToReceive = NULL;
        pxStreamBuffer->xTaskWaitingToSend = NULL;
        pxStreamBuffer->ucFlags = 0;
    }
    
    return ( StreamBufferHandle_t ) pxStreamBuffer;
}
```

### 2. 数据发送（写入）

```c
size_t xStreamBufferSend( StreamBufferHandle_t xStreamBuffer,
                         const void *pvTxData,
                         size_t xDataLengthBytes,
                         TickType_t xTicksToWait )
{
    StreamBuffer_t * const pxStreamBuffer = xStreamBuffer;
    size_t xSpace, xNextHead, xFirstLength;
    
    // 计算可用空间
    xSpace = prvBytesInBuffer( pxStreamBuffer );
    
    if( xSpace < xDataLengthBytes )
    {
        // 空间不足，可能需要阻塞等待
        if( xTicksToWait != 0 )
        {
            // 设置等待发送任务
            pxStreamBuffer->xTaskWaitingToSend = xTaskGetCurrentTaskHandle();
            
            // 阻塞当前任务
            vTaskPlaceOnEventList( &xStreamBufferWaitingTasks,
                                 xTicksToWait );
            
            // 任务切换
            taskYIELD();
        }
        else
        {
            return 0;  // 不等待，直接返回
        }
    }
    
    // 写入数据到缓冲区
    prvWriteBytesToBuffer( pxStreamBuffer, pvTxData, xDataLengthBytes );
    
    // 更新写指针
    pxStreamBuffer->xHead = ( pxStreamBuffer->xHead + xDataLengthBytes ) % pxStreamBuffer->xLength;
    
    // 通知等待接收的任务
    sbSEND_COMPLETED( pxStreamBuffer );
    
    return xDataLengthBytes;
}
```

### 3. 数据接收（读取）

```c
size_t xStreamBufferReceive( StreamBufferHandle_t xStreamBuffer,
                            void *pvRxData,
                            size_t xBufferLengthBytes,
                            TickType_t xTicksToWait )
{
    StreamBuffer_t * const pxStreamBuffer = xStreamBuffer;
    size_t xReceivedLength = 0;
    
    // 检查是否有足够数据
    if( prvBytesInBuffer( pxStreamBuffer ) < xBufferLengthBytes )
    {
        if( xTicksToWait != 0 )
        {
            // 设置等待接收任务
            pxStreamBuffer->xTaskWaitingToReceive = xTaskGetCurrentTaskHandle();
            
            // 阻塞当前任务
            vTaskPlaceOnEventList( &xStreamBufferWaitingTasks,
                                 xTicksToWait );
            
            // 任务切换
            taskYIELD();
        }
        else
        {
            return 0;  // 不等待，直接返回
        }
    }
    
    // 从缓冲区读取数据
    xReceivedLength = prvReadBytesFromBuffer( pxStreamBuffer,
                                             pvRxData,
                                             xBufferLengthBytes );
    
    // 更新读指针
    pxStreamBuffer->xTail = ( pxStreamBuffer->xTail + xReceivedLength ) % pxStreamBuffer->xLength;
    
    // 通知等待发送的任务
    sbRECEIVE_COMPLETED( pxStreamBuffer );
    
    return xReceivedLength;
}
```

## 环形缓冲区算法

### 1. 可用空间计算

```c
static size_t prvBytesInBuffer( const StreamBuffer_t *pxStreamBuffer )
{
    // 计算缓冲区中已使用的字节数
    size_t xCount = pxStreamBuffer->xHead - pxStreamBuffer->xTail;
    
    // 处理环形缓冲区的回绕
    if( pxStreamBuffer->xHead < pxStreamBuffer->xTail )
    {
        xCount += pxStreamBuffer->xLength;
    }
    
    return xCount;
}

static size_t prvBytesFree( const StreamBuffer_t *pxStreamBuffer )
{
    // 总空间减去已使用空间
    return pxStreamBuffer->xLength - prvBytesInBuffer( pxStreamBuffer ) - 1;
}
```

### 2. 数据写入算法

```c
static void prvWriteBytesToBuffer( StreamBuffer_t *pxStreamBuffer,
                                  const uint8_t *pucData,
                                  size_t xCount )
{
    size_t xFirstLength = pxStreamBuffer->xLength - pxStreamBuffer->xHead;
    
    if( xCount > xFirstLength )
    {
        // 需要回绕：先写尾部，再写头部
        memcpy( &pxStreamBuffer->pucBuffer[ pxStreamBuffer->xHead ],
                pucData,
                xFirstLength );
        
        memcpy( pxStreamBuffer->pucBuffer,
                &pucData[ xFirstLength ],
                xCount - xFirstLength );
    }
    else
    {
        // 不需要回绕：直接写入
        memcpy( &pxStreamBuffer->pucBuffer[ pxStreamBuffer->xHead ],
                pucData,
                xCount );
    }
}
```

### 3. 数据读取算法

```c
static size_t prvReadBytesFromBuffer( StreamBuffer_t *pxStreamBuffer,
                                     uint8_t *pucData,
                                     size_t xMaxCount )
{
    size_t xAvailable = prvBytesInBuffer( pxStreamBuffer );
    size_t xCount = ( xAvailable < xMaxCount ) ? xAvailable : xMaxCount;
    size_t xFirstLength = pxStreamBuffer->xLength - pxStreamBuffer->xTail;
    
    if( xCount > xFirstLength )
    {
        // 需要回绕：先读尾部，再读头部
        memcpy( pucData,
                &pxStreamBuffer->pucBuffer[ pxStreamBuffer->xTail ],
                xFirstLength );
        
        memcpy( &pucData[ xFirstLength ],
                pxStreamBuffer->pucBuffer,
                xCount - xFirstLength );
    }
    else
    {
        // 不需要回绕：直接读取
        memcpy( pucData,
                &pxStreamBuffer->pucBuffer[ pxStreamBuffer->xTail ],
                xCount );
    }
    
    return xCount;
}
```

## 任务通知机制

### 1. 发送完成通知

```c
#define sbSEND_COMPLETED( pxStreamBuffer )\
    vTaskSuspendAll();\
    {\
        if( ( pxStreamBuffer )->xTaskWaitingToReceive != NULL )\
        {\
            // 使用任务通知唤醒等待接收的任务\
            ( void ) xTaskNotify( ( pxStreamBuffer )->xTaskWaitingToReceive,\ 
                                  ( uint32_t ) 0,\ 
                                  eNoAction );\
            ( pxStreamBuffer )->xTaskWaitingToReceive = NULL;\
        }\
    }\
    ( void ) xTaskResumeAll();
```

### 2. 接收完成通知

```c
#define sbRECEIVE_COMPLETED( pxStreamBuffer )\
    vTaskSuspendAll();\
    {\
        if( ( pxStreamBuffer )->xTaskWaitingToSend != NULL )\
        {\
            // 使用任务通知唤醒等待发送的任务\
            ( void ) xTaskNotify( ( pxStreamBuffer )->xTaskWaitingToSend,\ 
                                  ( uint32_t ) 0,\ 
                                  eNoAction );\
            ( pxStreamBuffer )->xTaskWaitingToSend = NULL;\
        }\
    }\
    ( void ) xTaskResumeAll();
```

## 中断安全版本

### 1. 从中断发送数据

```c
BaseType_t xStreamBufferSendFromISR( StreamBufferHandle_t xStreamBuffer,
                                    const void *pvTxData,
                                    size_t xDataLengthBytes,
                                    BaseType_t *pxHigherPriorityTaskWoken )
{
    StreamBuffer_t * const pxStreamBuffer = xStreamBuffer;
    size_t xSpace;
    
    // 禁用中断，计算可用空间
    UBaseType_t uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();
    
    xSpace = prvBytesInBuffer( pxStreamBuffer );
    
    if( xSpace >= xDataLengthBytes )
    {
        // 写入数据
        prvWriteBytesToBuffer( pxStreamBuffer, pvTxData, xDataLengthBytes );
        pxStreamBuffer->xHead = ( pxStreamBuffer->xHead + xDataLengthBytes ) % pxStreamBuffer->xLength;
        
        // 通知等待接收的任务
        sbSEND_COMPLETE_FROM_ISR( pxStreamBuffer, pxHigherPriorityTaskWoken );
        
        portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );
        return pdTRUE;
    }
    
    portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );
    return pdFALSE;
}
```

## 消息缓冲区模式

### 1. 消息缓冲区创建

```c
StreamBufferHandle_t xMessageBufferCreate( size_t xBufferSizeBytes )
{
    // 消息缓冲区是流缓冲区的特化版本
    StreamBufferHandle_t xStreamBuffer;
    
    // 创建流缓冲区，但设置消息缓冲区标志
    xStreamBuffer = xStreamBufferCreate( xBufferSizeBytes, 0 );
    
    if( xStreamBuffer != NULL )
    {
        // 设置消息缓冲区标志
        ( ( StreamBuffer_t * ) xStreamBuffer )->ucFlags |= sbFLAGS_IS_MESSAGE_BUFFER;
    }
    
    return xStreamBuffer;
}
```

### 2. 消息发送

```c
size_t xMessageBufferSend( MessageBufferHandle_t xMessageBuffer,
                          const void *pvTxData,
                          size_t xDataLengthBytes,
                          TickType_t xTicksToWait )
{
    StreamBuffer_t *pxStreamBuffer = xMessageBuffer;
    
    // 在数据前添加长度信息
    configMESSAGE_BUFFER_LENGTH_TYPE xLengthHeader = xDataLengthBytes;
    
    // 先发送长度信息
    xStreamBufferSend( pxStreamBuffer,
                      &xLengthHeader,
                      sbBYTES_TO_STORE_MESSAGE_LENGTH,
                      xTicksToWait );
    
    // 再发送实际数据
    return xStreamBufferSend( pxStreamBuffer,
                             pvTxData,
                             xDataLengthBytes,
                             xTicksToWait );
}
```

## 性能优化特性

### 1. 零拷贝优化

流缓冲区设计避免了不必要的数据拷贝：
- **直接内存操作**：使用 `memcpy` 直接操作缓冲区
- **环形缓冲区**：避免数据移动的开销
- **触发级别**：减少不必要的任务唤醒

### 2. 内存效率

- **紧凑存储**：流数据连续存储，无消息边界开销
- **动态大小**：支持任意长度的数据读写
- **内存复用**：环形缓冲区重复利用内存空间

### 3. 实时性保证

- **中断安全**：提供 FromISR 版本的操作函数
- **优先级继承**：通过任务通知机制实现
- **最小阻塞**：精确的触发级别控制

## 实际应用示例

### 1. 串口数据接收

```c
// 创建串口接收流缓冲区
StreamBufferHandle_t xUartRxBuffer = xStreamBufferCreate( 1024, 64 );

// 串口中断服务程序
void UART_IRQHandler( void )
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t ucData;
    
    // 读取串口数据
    ucData = UART->DR;
    
    // 发送到流缓冲区
    xStreamBufferSendFromISR( xUartRxBuffer, &ucData, 1, &xHigherPriorityTaskWoken );
    
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

// 数据处理任务
void vDataProcessingTask( void *pvParameters )
{
    uint8_t ucBuffer[ 64 ];
    size_t xReceivedBytes;
    
    while( 1 )
    {
        // 等待数据到达（至少64字节才处理）
        xReceivedBytes = xStreamBufferReceive( xUartRxBuffer,
                                              ucBuffer,
                                              sizeof( ucBuffer ),
                                              portMAX_DELAY );
        
        if( xReceivedBytes > 0 )
        {
            // 处理接收到的数据
            processUartData( ucBuffer, xReceivedBytes );
        }
    }
}
```

### 2. 网络数据包处理

```c
// 创建消息缓冲区用于网络包传输
MessageBufferHandle_t xNetworkBuffer = xMessageBufferCreate( 2048 );

// 网络接收任务
void vNetworkReceiveTask( void *pvParameters )
{
    uint8_t pucPacket[ 256 ];
    size_t xPacketLength;
    
    while( 1 )
    {
        // 接收完整的数据包（包含长度信息）
        xPacketLength = xMessageBufferReceive( xNetworkBuffer,
                                              pucPacket,
                                              sizeof( pucPacket ),
                                              portMAX_DELAY );
        
        if( xPacketLength > 0 )
        {
            // 处理网络数据包
            processNetworkPacket( pucPacket, xPacketLength );
        }
    }
}
```

## 配置要求

### 必需配置

```c
// 必须启用任务通知功能
configUSE_TASK_NOTIFICATIONS = 1

// 消息长度类型定义（默认为 size_t）
#ifndef configMESSAGE_BUFFER_LENGTH_TYPE
    #define configMESSAGE_BUFFER_LENGTH_TYPE size_t
#endif
```

## 总结

`stream_buffer.c` 提供了 FreeRTOS 中高效的数据流传输机制：

1. **灵活的流处理**：支持任意字节数的读写操作
2. **高效的环形缓冲区**：内存利用率高，性能优异
3. **完善的任务同步**：通过任务通知实现精确的阻塞/唤醒
4. **中断安全操作**：提供 FromISR 版本支持实时应用
5. **消息缓冲区扩展**：在流缓冲区基础上实现消息传输

这种设计特别适合需要处理连续数据流的嵌入式应用，如通信协议栈、传感器数据采集、文件传输等场景。
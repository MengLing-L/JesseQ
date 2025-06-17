# JesseQ: 面向任意域上电路的高效零知识证明系统
>
> 我将简单介绍我们最近发表在 IEEE S&P(Oakland) 2025 上的文章 JesseQ: Efficient Zero-Knowledge Proofs for Circuits over Any Field
>
基于向量不经意线性评估（VOLE）的零知识证明（ZKPs）近期因其高效性和可扩展性获得显著关注。最新实现方案每秒可证明数千万逻辑门，并能平滑扩展至万亿级逻辑门验证。相较于现有简洁零知识证明系统，这类方案显著降低了证明方的计算开销。具体而言，工作LPZKv2表明在验证百万级逻辑门时，基于VOLE的ZKPs比现有常数轮方案（如Groth16、Virgo、Cerberus）快3-10倍。

基于VOLE的常数轮ZKPs性能如下表所示。在该范式下，线点零知识协议（LPZK）首次实现大域算术电路中每个逻辑门约1个通信元素的里程碑，后续QuickSilver将其改进为支持任意数域。另一改进方向LPZKv2则将通信量减半并降低计算成本。这些方案的通信复杂度与电路规模呈线性关系。AntMan首次实现 $\mathcal{O}(|C|^{3/4})$ 的亚线性通信构造（ $|\mathcal{C}|$ 为大域算术电路规模），虽然计算成本仍保持高效，但引入了 $\mathcal{O}(\log |C|)$ 的开销。在计算效率方面，QuickSilver和LPZKv2分别在布尔电路和大域算术电路中表现最优。值得注意的是，支持任意数域的VOLE-ZKPs与仅适用于大域的方案存在显著性能差异。

另一研究方向针对特定结构电路（如表示析取式 $\mathcal{C}_1(w)=1\vee\mathcal{C}_2(w)=1\cdots\vee\mathcal{C}_B(w)=1$ 的 $B$ 分支子电路）进行优化。最先进的Batchman实现了批量析取式的亚线性通信和计算，其框架可通过黑盒方式实例化基于VOLE的常数轮ZKPs来处理乘法运算。
|   Boolean    |            |             |  Arithmetic  |            |             |
| :----------: | :--------: | :---------: | :----------: | :--------: | :---------: |
| Scheme&nbsp; | Size&nbsp; | Speed&nbsp; | Scheme&nbsp; | Size&nbsp; | Speed&nbsp; |
|  Wolverine   |     7      | 1.25 M/sec  |  Wolverine   |     4      | 0.96 M/sec  |
| Mac‘n‘Cheese |     -      |      -      | Mac‘n‘Cheese |     3      |  3.6 M/sec  |
|   IT-LPZK    |     -      |      -      |   IT-LPZK    |   2+1/t    | 19.6 M/sec  |
| QuickSilver  |     1      |  8.6 M/sec  | QuickSilver  |     1      |  7.8 M/sec  |
|  IT-LPZKv2   |     -      |      -      |  IT-LPZKv2   |   1+1/t    | 21.8 M/sec  |
|  ROM-LPZKv2  |     -      |      -      |  ROM-LPZKv2  |    1/2     |  9.8 M/sec  |
|    AntMan    |     -      |      -      |    AntMan    | sublinear  | 7.01 M/sec  |
|     JQv1     |     1      | 64.1 M/sec  |     JQv1     |     1      | 23.3 M/sec  |
|     JQv2     |    1/2     | 34.2 M/sec  |     JQv2     |    1/2     | 13.7 M/sec  |
- 本表格将我们的工作（JQv1 和 JQv2）与先前相关工作的证明者成本进行了对比，数据来源包括他们研究中报告的结果以及在相同硬件上运行的实验（AntMan 除外，其使用了更高配置的实例）。
  
- **Size** 表示每个乘法门需要发送的域元素数量（AntMan 除外，其具有亚线性通信特性）。  
  **Speed** 表示在无限带宽和单线程条件下每秒可执行的乘法门数量（AntMan除外，其使用1 Gbps网络带宽并在四线程下运行）。  

- 此外，其他工作将算术电路的素数设为 $p = 2^{61} -1$，而由于同态加密的特性，AntMan 要求将 $p$ 设为更小的素数 $2^{59} - 2^{28} + 1$。

 ## JesseQ的贡献
我们提出JesseQ（JQv1与JQv2）——一种基于VOLE的常数轮零知识证明新方案，适用于预处理模型中任意数域上的电路（与LPZKv2框架相同）。其中JQv1支持任意结构电路，JQv2针对分层电路优化，二者均具备更优的在线阶段性能。如既往研究，我们重点分析证明方效率：对于任意数域电路，JQv1/JQv2的证明方与验证方计算量均优于现有最优方案。唯一例外是：在大域电路场景下，JQv1验证方每个乘法门需比LPZKv2多执行一次乘法运算。但我们的协议仍通过全数域兼容性保持整体优势。

## 基于VOLE的零知识证明 

我们首先回顾先前工作的相关背景，再对协议的技术概要进行说明。

### 消息认证码与VOLE

大多数基于VOLE的零知识证明将VOLE关联性用作信息论消息认证码（IT-MAC，使得证明者（Prover）能够向验证者（Verifier）提交线值承诺。  
设 $\mathbb{F}_p$ 为有限域， $x \in \mathbb{F}_p$ 是验证者已知的全局密钥， $u\in\mathbb{F}_p$ 是证明者已知的值。对 $u$ 的IT-MAC承诺由一对值 $(m, k)$ 组成，其中 $m$ 由证明者持有， $k$ 由验证者持有，且满足关系式 $m = k - u\cdot x$。  

- **承诺打开阶段**：证明者发送 $(m, u)$ 给验证者，验证者检查是否满足 $m = k - u \cdot x$。  
- **符号表示**：记全局密钥 $x$ 下对 $u$ 的承诺为 $[u]$，也称 $[u]$ 为认证值， $m$ 是 $u$ 的MAC标签。  
- **安全性**：该MAC具有信息论隐藏性和绑定性。  
- **同态性**：支持对承诺值的同态加法运算，即 $[a+b] = [a] + [b]$。  
- **公开常数处理**：对于公开常数 $c$，其MAC定义为 $(m:=0, c)$ 和 $(k:=c\cdot x, x)$，从而支持任意来自承诺值的仿射运算。  

#### VOLE功能  
VOLE功能允许证明者和验证者共同构造一组认证随机值。调用后：  
- 证明者获得两个随机域元素向量 ( ${m},{u}$ )；  
- 验证者获得全局密钥 $x$ 和向量 ${k}$ ，且满足 ${k} = {m} + {u} \cdot x$ 。  
该过程实现了对随机向量 $[{u}]$ 的认证。  

#### 承诺转换  
若证明者已知某向量 $[{w}]$ 的承诺，可将随机向量 $[{u}]$ 的承诺转换为 $[{w}]$ 的承诺：  
1. 证明者发送差值向量 ${d} := {w} - {u}$ 给验证者；  
2. 验证者更新 ${k}$ 为 ${k} + {d}\cdot x$。  

### 基于VOLE的零知识证明框架 

大多数恒定轮次的基于VOLE的零知识证明遵循"承诺-证明"框架，并包含预处理阶段以提高效率。具体包含以下阶段：

1. **预处理阶段**  
   Prover 和 Verifier 调用VOLE功能，获得对随机向量 ${u}$ 的承诺 $[{u}]$，这些值将在在线阶段被消耗。  
   *（对应步骤标签 `step:1`）*

2. **在线阶段 - 承诺**  
   Prover 将电路中所有线值承诺给 Verifier：  
   - 对于电路的输入线和乘法门的输出线，Prover 发送差值向量 ${d}:= {w} - {u}$  
   - 通过该操作，将预处理阶段生成的随机值承诺转换为线值承诺  
   - 由于IT-MAC的加法同态性，加法门输出线的承诺可由双方在本地计算  
   - 发送的域元素总数 = 乘法门数量 + 输入线数量  
   *（对应步骤标签 `step:2`）*

3. **在线阶段 - 证明[乘法正确性]**  
   Prover 和 Verifier 协作验证每个乘法门输出线 $[w]$ 的计算正确性。  
   *（对应步骤标签 `step:3`）*

4. **在线阶段 - 证明[输出正确性]**  
   - Prover 发送电路输出线的MAC标签 $m$ 给 Verifier
   - 当输出线 $w$ 预期为1时，Verifier 验证 $k = m + x$ 是否成立

**核心差异**：现有恒定轮次VOLE-ZKP方案的主要区别在于步骤 `step:3` 的实现方式。

### 乘法验证：LPZK与QuickSilver方案
LPZK 及其后续改进方案 QuickSilver采用了相似的思路来验证乘法运算的正确性。下面我们将具体阐述 Prover 如何在承诺线值后向 Verifier 证明乘法门的正确计算。假设某乘法门的输入线为 $w_{\alpha},w_{\rho}$ ，输出线为 $w_{\upsilon}$ ，给定承诺值 $[w_{\alpha}], [w_{\rho} ], [w_{\upsilon}]$ ，Prover 需证明 $w_{\alpha} \cdot w_{\rho} = w_{\upsilon}$ 。

**核心思想**：是将IT-MACs解释为$X$的线性多项式：定义 $p_{i}(X) = m_{w_i} + w_i\cdot X$ （其中  $i\in\{\alpha, \rho, \upsilon\}$ ），构造二次多项式 $f(X) = p_{\alpha}(X)\cdot p_{\rho}(X) - X\cdot p_{\upsilon}(X)$ 。Verifier 已知其在 $x$ 处的取值 $f(x) = k_{w_\alpha}\cdot k_{w_\rho} - x \cdot k_{w_\upsilon}$ ，而 Prover 掌握其系数。具体展开为：
$$f(X)= a_0 + a_1\cdot X + a_2\cdot X^2$$
其中系数分别为：
- $a_0 = m_{w_{\alpha}}\cdot m_{w_{\rho}}$
- $a_1 = w_{\alpha}\cdot m_{w_{\rho}} + w_{\rho}\cdot m_{w_{\alpha}} - m_{w_{\upsilon}}$
- $a_2 = w_{\alpha}\cdot w_{\rho} - w_{\upsilon}$

**关键性质**：当且仅当 $w_{\alpha}\cdot w_{\rho} = w_{\upsilon}$时， $f(X)$ 退化为线性多项式。为证明这一点，Prover可发送 $a_0$ 和 $a_1$ 给Verifier，后者验证 $a_0+ a_1\cdot x \stackrel{?}{=} k_{w_\alpha}\cdot k_{w_\rho} - x \cdot k_{w_\upsilon}$ 。但直接发送每个门的两个域元素效率低下，因此QuickSilver采用批量验证策略：

**批量验证流程**：
1. 设有 $L$ 个乘法门对应线性多项式 $f_j(X)$ （ $j=1$ 至 $L$ ）
2. Verifier发送随机挑战值 $\chi$ 
3. Prover计算聚合多项式 $F(X):=\sum_{j=1}^L f_j(X) \cdot \chi^j = A + B\cdot X$ 并发送 $A,B$ 
4. Verifier验证 $A+B\cdot x \stackrel{?}{=} \sum_{j=1}^L f_j(x) \cdot \chi^j$ 

*注：为实现零知识性，需额外VOLE关联值来掩盖 $A$ 和 $B$ ，此处暂不展开。

**小域场景的增强**：
上述方案在大域算术电路上有效，但在小域存在安全风险， 攻击者可非不可忽略概率猜测全局密钥 $x$ 从而伪造IT-MAC。为此，QuickSilver引入子域VOLE（sVOLE）变体，其关联随机数形式为 $k = m + u \cdot x$  ，其中 $u  \in  \mathbb{F}^n_{p}$ ， $k , m \in \mathbb{F}^n_{p^r}$ ， $x \in \mathbb{F}_{p^r}$ 。

### 乘法验证：LPZKv2改进方案
LPZKv2通过两项技术改进实现了对LPZK的优化：

**技术改进1：IT-MAC重构**  
将见证值置于 $m$ 而非 $u$ 中。新架构下：
- 认证 $m$ 时，Prover持有 $u$ ，Verifier持有 $k$ 和 $x$ ，满足 $k=m+u\cdot x$ 
- 优势：Verifier在收到Prover发送的 $d:=w - m$ 后，可直接通过 $k + d$ 更新 $k$ （原方案需计算 $k+ d\cdot x$ ），每门节省一次乘法运算

**技术改进2：二次VOLE（qVOLE）**  
通过生成额外关联随机数提升在线阶段效率。除基础关联式 $k_i = m_i + u_i \cdot x$ （ $i\in\{\alpha, \rho, \upsilon\}$ ）外，qVOLE还生成：
$$k_y = m_y + u_y \cdot x \quad \text{其中} \quad u_y = u_{\alpha}\cdot u_{\beta}$$
该机制显著降低复杂度，具体原理如下：

**乘法验证流程**  
1. **承诺阶段**：采用常数项承诺形式 $k_i=w_i + u_i \cdot x$ （ $i\in\{\alpha, \rho, \upsilon\}$ ）
2. **预处理阶段**：通过qVOLE获取 $(m_y, u_y)$ （Prover）和 $(k_y, x)$ （Verifier），满足 $u_y = u_{\alpha}\cdot u_{\rho}$ 
3. **多项式构造**：定义二次多项式
   $$f(X) = p_{\alpha}(X)\cdot p_{\rho}(X) - p_{\upsilon}(X) - X \cdot p_y(X)$$
   - Verifier已知 $f(x) = k_{w_\alpha}\cdot k_{w_\rho} - k_{w_\upsilon} - x \cdot k_y$ 
   - Prover掌握其系数：
     $$f(X) = a_0 + a_1\cdot X + a_2\cdot X^2$$
     其中：
     - $a_0 = w_{\alpha}\cdot w_{\rho} - w_{\upsilon}$
     - $a_1 = w_{\alpha}\cdot u_{w_{\rho}} + w_{\rho}\cdot u_{w_{\alpha}} - u_{\upsilon} - m_y$
     - $a_2 = u_{\alpha} \cdot u_{\rho} - u_y \equiv 0$（因 $u_y=u_{\alpha}\cdot u_{\rho}$ ）

4. **验证阶段**：
   - 当且仅当 $w_{\alpha}\cdot w_{\rho} = w_{\upsilon}$ 时 $a_0=0$ 
   - Prover发送 $a_1$ ，Verifier验证 $a_1\cdot x \stackrel{?}{=} k_{w_\alpha}\cdot k_{w_\rho} - k_{w_\upsilon} - x\cdot k_y$ 

**批量验证协议**  
对于 $L$ 个需验证的常数多项式 $f_j(X) / X = a_{j, 1}$ （Verifier已知 $f_j(x)$ ）：
1. Prover 计算并发送 $A = \prod_{j=1}^{L} a_{j, 1}$ 
2. Verifier 验证 $A \stackrel{?}{=} \prod_{j=1}^{L} f_j(x) / x$ 
*核心原理*：当且仅当所有多项式均为常数时，其乘积才是常数多项式。

**电路优化扩展**  
LPZKv2对特定电路类有额外优化。关键发现是表达式：
$$g(x)=p_{\alpha}(x)\cdot p_{\rho}(x) - x \cdot p_y(x) = a_0 + a_1\cdot x$$
其中：
- $a_0 = w_\alpha\cdot w_\rho$
- $a_1 = w_\alpha\cdot u_{\rho} + w_\rho\cdot u_{\alpha} - m_y$
天然构成对 $w_{\alpha} \cdot w_{\rho}$ 的认证，且无需任何通信开销。

## JesseQ方案技术概览 
本节阐述JesseQ的设计原理。我们通过二次子域VOLE(qsVOLE)技术融合QuickSilver与LPZKv2的优势，并设计了适配qsVOLE的高效乘法验证协议。相较于现有基于二次多项式的方法，我们的方案采用一次多项式实现更高效的验证，同时设计了创新的批量验证机制。

### 通用域电路方案JQv1
对于输入线 $(w_{\alpha}, w_{\rho})$ 和输出线 $w_{\upsilon}$ 的乘法门：

**预处理阶段**  
1. 双方通过qsVOLE获取随机认证值 $[u_\alpha], [u_\rho], [u_\upsilon]$ 及 $[y=u_\alpha\cdot u_\rho]$ 
2. Prover发送差值 $d_i := w_i - u_i$  （ $i \in {\alpha,\rho, \upsilon}$ ）
3. 双方计算 $[w_\upsilon] = [u_\upsilon] + d_\upsilon$ 

**验证多项式构造**  

1. **线性多项式定义**：  
   $p_{i}(X) = m_{u_i} + u_i\cdot X$ （ $i\in \{\alpha,\rho \}$ ）  
   $p_{\upsilon}(X) = m_{w_\upsilon} + w_\upsilon\cdot X$  
   $p_y(X) = m_y + y\cdot X$

2. **一次多项式构造**：  
   $f(X) = d_\rho\cdot p_{\alpha}(X)+ d_\alpha\cdot p_{\rho}(X) + p_y(X) + d_\rho\cdot d_\alpha\cdot X - p_v(X)= a_0 + a_1\cdot X$

其中：
- $a_0 = d_{\rho}\cdot m_{u_{\alpha}} + d_{\alpha}\cdot m_{u_{\rho}} + m_{y}- m_{w_\upsilon}$
- $a_1 = (u_{\alpha} + d_{\alpha})\cdot(u_{\rho} + d_{\rho}) - w_\upsilon$

**验证逻辑**  
当且仅当乘法门计算正确时：
$$a_1=w_{\alpha} w_{\rho} - w_{\upsilon} = 0$$
此时 $f(X)$ 退化为常数多项式。虽然Prover可发送 $a_0$ 供Verifier验证 $a_0 \stackrel{?}{=}f(x)$ ，但会导致每门传输一个域元素。

#### 高效批量验证协议
我们采用随机预言机模型下的哈希函数 $\mathsf{H}$ 实现批量验证：
1. 对于 $L$ 个待验证常数多项式 $f_j(X)=a_{0,j}$ 
2. Prover 计算并发送 $h=H(a_{0,1} || \cdots || a_{0,L})$ 
3. Verifier 验证 $h \stackrel{?}{=} H(f_1(x)||\cdots||f_L(x))$ 

**方案扩展性**  
如LPZKv2，本方案可从标准算术电路推广至任意二次多项式门电路[[详见文章第3节]](https://eprint.iacr.org/2025/533)。

#### 技术优势说明：  
1. **布尔电路优化**：一次多项式 $f(X)$ 的构造避免了扩域元素乘法运算。在布尔电路中，标量乘法， 如 $d_{\alpha} \cdot m_{u_{\rho}}$ ，比扩域乘法，如 $m_{w_{\rho}}\cdot m_{w_{\alpha}}$ 效率显著提升
2. **批量验证效率**：相比QuickSilver与LPZKv2分别需要的 $\mathbb{F}_{p^r}$ 三次乘法和 $\mathbb{F}_p$ 一次乘法，基于哈希的验证具有更优实际性能。例如在Amazon EC2m5.2xlarge 实例上，BLAKE3 处理1000万61/128位域元素的速度至少是同规模域乘法运算的2倍[[详见文章第6.4节]](https://eprint.iacr.org/2025/533)


### JQv2 面向任意域上的分层电路

JQv2 将分层电路中每个乘法门由证明者发送的域元素数量（均摊值）降低至 $\frac{1}{2}$ 。这一优化基于与 LPZKv2 相似的观察： 
$g(X) = d_\rho\cdot p_{\alpha}(X)+ d_\alpha\cdot p_{\rho}(X) + p_y(X) + d_\rho\cdot d_\alpha\cdot X = a_0 + a_1\cdot X$

其中：
- $a_0 = d_{\rho} \cdot m_{u_{\alpha}} + d_{\alpha} \cdot m_{u_{\rho}} + m_{y}$
- $a_1 = (u_{\alpha} + d_{\alpha}) \cdot (u_{\rho} + d_{\rho})$
  
该多项式已能直接认证 $w_{\alpha} \cdot w_{\rho}$ 而无需任何通信。为简化描述，此处假设偶数层的导线数量多于奇数层。具体而言，我们允许证明者和验证者直接使用输入导线关联的值计算偶数层门输出的 $[w_{\upsilon}]$，从而避免证明者向验证者发送 $d_{\upsilon}$。但对于奇数层的门，证明者仍需发送 $d_{\upsilon}$ 供验证者计算 $[w_{\upsilon}]$。由于奇数层输入（偶数层输出）的关联值不再是随机值 $u$ 与双方已知值 $d$ 的简单线性组合，JQv1 中的校验方法在此不适用。

因此，在给定输入输出导线的认证值后，我们采用与 QuickSliver 相似的基于二次多项式在 $x$ 处求值的校验方法，具体细节在 [[文章第4节]](https://eprint.iacr.org/2025/533)中阐述。


>
> 作者&联系方式：刘梦玲（https://mengling-l.github.io/）
>
 ## 参考文献
我们的在线版本: https://eprint.iacr.org/2025/533

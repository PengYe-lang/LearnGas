# Aura 的 GAS 架构与 LearnGas 学习路线

读 Aura，最值得追的是一发火球如何从鼠标输入变成敌人的一次掉血。玩家控制器知道鼠标点在哪里，技能知道何时发射，投射物知道撞到了谁，伤害计算知道如何处理抗性和护甲，属性集决定这次扣血是否致死。换成敌人的远程攻击时，可以替换前半段的决策与瞄准，保留后半段的结算。这是 Aura 最值得 LearnGas 借鉴的设计。

**取舍建议**：先学它的 ASC 所有权、输入标签和伤害流水线。进入多人阶段前，优先修正效果上下文的类型与序列化、范围伤害回调的生命周期，以及技能加点和换槽的服务端一致性。直接照搬这些实现，单机中看起来正确的结果可能在远端玩家处失真。

**分析范围**：本机外层 `GameplayAbilitySystem_Aura-main/Aura.uproject` 声明 UE 5.3，包含完整的 `Source`、`Config` 和 `Content`。同名嵌套目录缺少源码与内容。下文的文件定位均相对此完整工程；C++ 简写路径默认在 `Source/Aura/Private/` 下。结论来自本地源码、配置、蓝图包内的资源及函数引用，以及 Epic 文档。蓝图引用可以确认依赖，无法单凭名称确定执行引脚顺序。多人场景列出的是验收方法，本次未运行 Aura 的多人 PIE。

## 1. 谁持有状态，谁可以改变状态

| 对象 | Aura 的职责 | 设计含义 |
| --- | --- | --- |
| `AAuraPlayerState` | 创建玩家的 `UAuraAbilitySystemComponent`（ASC）和 `UAuraAttributeSet`。另外复制等级、XP、属性点和技能点 | 角色身体与玩家进度有不同寿命。换 Pawn 时可以保留同一个 ASC；跨地图或重开游戏的恢复另走 SaveGame |
| `AAuraCharacter` | 把 PlayerState 设为 GAS 的 Owner、当前角色设为 Avatar，向 HUD 和表现组件公布 ASC | 可以从角色进入 GAS，但属性值、技能授权与复制各有明确的归属 |
| `AAuraEnemy` | 敌人自身持有 ASC 和属性集，Owner 与 Avatar 都是敌人。行为树在服务端启动，黑板接收受击与眩晕状态 | 敌人和玩家能吃同一个伤害 GE。普通敌人的技能状态随该 Actor 一同销毁 |
| `UAuraAbilitySystemComponent` | 管理已授予的技能规格、输入、技能等级、状态、槽位和通知 | `FGameplayAbilitySpec` 表示“某人拥有的这项技能”。同一火球技能类可以对应不同玩家的等级与键位 |
| `UAuraAttributeSet` | 保存属性，消费伤害和经验中间值，触发死亡、受击、升级和显示反馈 | 集中处理生命上限与死亡边界，有利于教学。随着规则增加，这里也是最先出现职责拥挤的位置 |
| `AAuraHUD` 与 WidgetController | HUD 将 PC、PS、ASC、AS 注入控制器；Overlay、属性菜单和技能菜单分别订阅所需数据 | Widget 得到可显示的数据和可发出的操作。敌人血条更轻，敌人自己就充当 WidgetController |

### 初始化为何有两个入口

玩家在服务端从 `AAuraCharacter::PossessedBy` 进入 `InitAbilityActorInfo`，再调用 `LoadProgress`。首次进入时初始化默认属性并授予技能；从存档进入时重建技能规格、恢复等级点数，再从主属性恢复派生属性。客户端在 `OnRep_PlayerState` 中等待 PlayerState 就绪，然后绑定同一套 ActorInfo。两端都需要知道技能当前驱动哪个身体，授予和恢复则由服务端负责。查阅 `Character/AuraCharacter.cpp` 和 `Player/AuraPlayerState.cpp`。

敌人在 `BeginPlay` 中初始化 ActorInfo，服务端再按职业和等级授予技能。玩家 ASC 使用 **Mixed**，敌人使用 **Minimal**。这控制活动 Gameplay Effect 的复制范围：Mixed 让拥有者获取较完整的活动效果信息，Minimal 主要让远端通过标签和 Cue 感知效果。生命等属性仍由 AttributeSet 的属性复制与 RepNotify 驱动。远端血条可以更新，不意味着它也持有目标的完整效果列表。查阅 `Character/AuraEnemy.cpp` 与 `AbilitySystem/AuraAttributeSet.cpp`。

这个取舍与 Lyra 的共通点是把玩家长期 GAS 状态放在 PlayerState。Lyra 进一步记录 Pawn 或装备授予的句柄，在解除附身时按来源撤销。对于 LearnGas，第一次需要“换身体保留经验，但卸下旧身体的冲刺技能”时，引入这套记账才有明确收益。[Epic：Abilities in Lyra](https://dev.epicgames.com/documentation/en-us/unreal-engine/abilities-in-lyra-in-unreal-engine)

## 2. 一发火球的完整调用链

| 步骤 | 本地源码或资产入口 | 数据如何交接 |
| --- | --- | --- |
| ① 输入分流 | `Player/AuraPlayerController.cpp`，`DA_AuraInputConfig` | Enhanced Input 的动作映射为输入标签。左键同时承担移动和施法，控制器根据目标和 Shift 状态分流，并检查眩晕等输入阻断标签 |
| ② 找到玩家的技能 | `UAuraAbilitySystemComponent::AbilityInputTagPressed/Held/Released` | ASC 按 Spec 的动态输入标签精确匹配。Held 对未激活的匹配项调用 `TryActivateAbility`；已激活技能的按下、松开事件可按 SpecHandle 和预测键送给等待输入的任务 |
| ③ 取得瞄准数据 | `AbilitySystem/AbilityTasks/TargetDataUnderMouse.cpp` | 本地玩家在预测窗口内做鼠标射线，把命中包装成 `SingleTargetHit` 发往服务器。远端任务按本次激活的句柄和预测键等待，数据先到时也可立即消费 |
| ④ 编排施法 | `Content/Blueprints/AbilitySystem/Aura/Abilities/Fire/FireBolt/GA_FireBolt.uasset` | 包内可核对 `CommitAbility`、鼠标任务、蒙太奇、等待 GameplayEvent、`SpawnProjectiles`、`EndAbility`，以及费用、冷却和伤害 GE 的引用。取消、打断及扣费时机需要在蓝图图表上沿执行线核查 |
| ⑤ 发射并确认命中 | `UAuraFireBolt::SpawnProjectiles`，`AAuraProjectile::OnSphereOverlap` | 服务端延迟生成可复制的投射物，在完成生成前填入 `FDamageEffectParams`。命中时过滤来源和友方，权威端取得目标 ASC，补上冲量并调用 `ApplyDamageEffect` |
| ⑥ 结算与反馈 | `UAuraAbilitySystemLibrary::ApplyDamageEffect` → `UExecCalc_Damage` → `UAuraAttributeSet::HandleIncomingDamage` | 源 ASC 创建 Context 和 GE Spec，把本次数值写入 Spec，送给目标 ASC。执行计算输出 `IncomingDamage`；属性集扣血，驱动受击、死亡、飘字和异常状态 |

这里有三个容易混淆的“规格”。**AbilitySpec** 随玩家保存技能等级与槽位；**GameplayEffectSpec** 表示某一次具体的效果应用，携带等级及 SetByCaller 数值；**EffectContext** 记录这次应用的来源和事件信息，例如是否暴击、击退方向。两名玩家同时释放同一级火球，也应拥有独立的命中上下文。预测键用来对应一次激活；目标距离、阵营、存活状态等玩法条件仍需由权威规则判断。[Epic：Gameplay Abilities](https://dev.epicgames.com/documentation/unreal-engine/using-gameplay-abilities-in-unreal-engine) · [Epic：Ability Tasks](https://dev.epicgames.com/documentation/unreal-engine/gameplay-ability-tasks-in-unreal-engine)

## 3. 数值如何从配置变成一次掉血

**先看配置的入口。** `AuraAssetManager::StartInitialLoading` 注册原生标签并初始化 GAS 全局数据；`DefaultEngine.ini` 指定自定义 AssetManager，`DefaultGame.ini` 配置自定义 AbilitySystemGlobals 和 GameplayCue 搜索路径。Globals 负责分配 `FAuraGameplayEffectContext`。只把伤害类复制进另一个工程而遗漏这个入口，普通 Context 与项目自定义访问器的类型假设就会冲突。

| 变化的东西 | Aura 选用的载体 | 读代码时的例子 |
| --- | --- | --- |
| 角色或技能的静态定义 | Data Asset、Curve Table | `DA_CharacterClassInfo` 对应职业默认值；`DA_AbilityInfo` 用于技能目录；`CT_DamageCalculationCoefficients` 提供穿甲、有效护甲和暴击抗性系数 |
| 一次施法传入的参数 | GE Spec 的 SetByCaller | `ApplyDamageEffect` 用伤害类型 Tag 写入基础伤害，另写异常概率、每跳伤害、持续时间和周期 |
| 单个派生量 | Modifier Magnitude Calculation（MMC） | `MMC_MaxHealth` 捕获目标 Vigor，并从 SourceObject 的战斗接口取等级。该工程公式为 `80 + 2.5 × Vigor + 10 × 等级`；因此构造初始化 GE 时，SourceObject 的约定也属于公式输入 |
| 多方参与的结算 | Execution Calculation | `ExecCalc_Damage` 捕获源的穿甲、暴击率等和目标的护甲、格挡、抗性等，捕获定义采用非快照。投射物在途期间的属性变化应按实际创建和执行 Spec 的时点理解 |
| 可持续观察的状态与一次性表现 | 持续 GE、Granted Tag、GameplayCue、Niagara | 灼烧的周期 GE 再写入 `IncomingDamage`；眩晕标签影响输入和移动。Cue 资产有 `GC_MeleeImpact`、`GC_ShockLoop`；被动光环还使用项目自己的 Niagara 通知链 |

**伤害顺序值得手算一次。** 执行类先尝试判定异常，再逐个伤害类型应用对应抗性与可选范围衰减，汇总后判格挡（减半）、护甲与穿甲，最后判暴击（翻倍再加暴击附加值）。例如基础火伤 100、火抗 20%、发生格挡、护甲修正为零、没有暴击，输出应为 **40 点 IncomingDamage**。若在同样条件下暴击且附加值为 10，输出应为 **90**。这两个例子能检查调整公式顺序有没有悄悄改变游戏设计。

属性集读出 `IncomingDamage` 后先清零，再把生命夹在零与最大生命之间。非致死命中可激活受击技能或产生击退。致死命中走战斗接口并发出经验事件；`GA_ListenForEvent` 包内可查到 `Attributes.Meta.IncomingXP` 和事件效果引用。经验进入 `HandleIncomingXP` 后按升级表计算跨过的等级、奖励属性点与技能点，并公布升级。这个间接层使治疗、护盾或经验奖励都能有独立规则；同时也意味着调试时应分别观察“中间值是否产生”和“中间值是否被消费”。[Epic：Gameplay Effects、Executions 与 Gameplay Cues](https://dev.epicgames.com/documentation/unreal-engine/gameplay-effects-for-the-gameplay-ability-system-in-unreal-engine)

## 4. 技能树、界面和存档共享了什么

**技能树。** `UpdateAbilityStatuses` 遍历技能定义；达到等级且尚未授予时，创建带 `Eligible` 状态的 Spec。花技能点可将其变为 `Unlocked`，已解锁或装备的技能则提升 Spec 等级。装备再写入 `Equipped` 和输入槽。未授予的锁定技能也能在目录展示，因为“游戏中定义了这项技能”和“玩家拥有该 Spec”是两个查询。主动技能和被动技能共用目录，被动技能在装备与换下时另有激活、结束和视觉通知。

**界面。** `AuraHUD::InitOverlay` 注入依赖，`OverlayWidgetController` 先提供初始值，随后监听属性、XP、技能授予、装备及效果资产标签；例如带 `Message` 的效果可以查询消息表生成提示。技能菜单负责显示可升级、可装备状态并发出操作请求。`UWaitCooldownChange` 监听活动效果加入和冷却 Tag 移除；有多个匹配效果时取最长剩余时间。注意它只在新增事件后查询剩余时间，因此中途打开面板能否立即显示现有冷却，需要 UI 初始化主动查询。载入界面则有独立的 `MVVM_LoadScreen` 和 `MVVM_LoadSlot`，战斗 HUD 使用 WidgetController。

**存档。** `AAuraCharacter::SaveProgress_Implementation` 记录等级、经验、两类点数、四项主属性和每个技能的类、Tag、类型、等级、状态、槽位；读档以这些值重建 Spec，按状态恢复已装备被动。世界快照由 `AAuraGameModeBase::SaveWorldState/LoadWorldState` 按地图和 Actor 名称匹配，序列化标记为 SaveGame 的字段。其语义更接近“从检查点恢复养成进度”。若希望恢复战斗中途的灼烧层数和冷却剩余时间，需要单独定义时间基准、来源和存档版本。换 Actor 名称也应考虑旧存档映射。

## 5. 源码中应先处理的问题

以下按对后续学习的影响排序。行号对应本机外层副本，函数名可用于定位其他版本。

### ① 自定义 EffectContext 有三处独立的正确性问题

`Source/Aura/Public/AuraAbilityTypes.h:113` 的 `GetScriptStruct` 返回基类的 `StaticStruct()`，而 Globals 实际创建的是派生结构。GAS 的句柄序列化使用这个反射类型选择结构与序列化器。紧接着的 `Duplicate` 又明确分配基类，复制后会切掉暴击、异常和冲量等派生字段。即使两处都修好，`AuraAbilityTypes.cpp:93` 只序列化 **19 位**标记，而非零范围中心占用 **bit 19**（第 20 位）。写端和读端可能因此对后续坐标载荷的存在产生分歧。三个问题必须分别处理。

**修复与验收**：返回派生 `StaticStruct`，用派生类型复制并独立复制可变命中数据；以统一的标记位枚举推导位数，保留内部序列化失败信息。做一次“复制 → 序列化 → 反序列化”的往返测试，同时核对类型、暴击标记、伤害 Tag、非零范围中心，以及后续字段的读取位置。只测世界原点会漏掉位数问题。

### ② 范围衰减的计算留下了悬空回调

`AbilitySystem/ExecCalc/ExecCalc_Damage.cpp:188` 把引用捕获局部 `DamageTypeValue` 的 lambda 绑定到目标的长期伤害委托，再调用世界范围伤害函数以回读衰减值。函数离开该迭代后，绑定仍然存在。`Character/AuraCharacterBase.cpp:66` 的 `TakeDamage` 会继续广播到这些回调。后续广播存在访问已失效局部变量的风险。并且对每个目标、每种伤害分量触发世界级范围查询，会把一次局部数值计算放大成场景操作。

**修复与验收**：一次爆炸先完成目标筛选、遮挡与距离采样，再给每个目标生成一次 Spec，传入该目标的衰减系数，让执行类只计算它自己的伤害。用内圈、中点、外圈和被遮挡的四个目标验证衰减规则；相继发生两次爆炸时，每个目标的命中次数应与设计相符。

### ③ 加点和换槽的服务端状态变更缺少完整校验

`AuraAbilitySystemComponent.cpp:278` 的属性升级在调用端检查点数，服务端处理函数发送事件并扣点；第 310 行的技能升级找到 Spec 后就扣点，扣点发生在状态分支之前。换槽界面的 `SpellRowGlobePressed` 会比对技能与槽位类型，服务端仍应独立检查。第 368 行清掉被替换技能的槽位，随后只显式标脏新技能；旧项仍可保留 `Equipped`。第 385 行的装备成功通知又位于合法状态分支之外。这样会让点数、Spec、菜单、被动表现和存档对“已装备”持有不同答案。

**修复与验收**：在权威端按“检查余额、精确技能标识、合法状态与槽位 → 一次修改涉及的 Spec 和余额 → 标脏每个变化项 → 通知结果”处理。测试最后一点的消费、同槽换技、被动换下再读档；对照服务端和拥有者客户端，各槽最多一项，余额不为负，失败操作保持原状态。

### ④ 远端玩家的锁定技能说明依赖 GameMode

`SpellMenuWidgetController::SpellGlobeSelected` 会请求技能描述。已有 Spec 时可从技能类生成说明；对有效但尚未授予的技能，`GetDescriptionsByAbilityTag` 会读取 `GetAbilityInfo` 的结果。后者在 `AuraAbilitySystemLibrary.cpp:204` 从 GameMode 取资产，普通网络客户端没有权威 GameMode，该分支会解引用空指针。仅使用主机的技能菜单很容易错过这个问题。

**修复与验收**：用双方都能加载的只读技能目录生成名称、说明和等级要求，例如让 WidgetController 使用已注入的 AbilityInfo；个人等级与解锁状态继续来自复制的 Spec。用独立客户端分别选择未达到等级、已解锁和已装备的技能，三种情况都应能稳定预览。

### ⑤ 周期异常的对象所有权和概率边界值得修正

`AuraAttributeSet.cpp:218` 每次根据本次命中创建临时的持续 GE 定义，并设置按来源堆叠、上限一层。第 245 行用裸 `new` 创建 Spec，按引用应用后没有对应释放。这个 Spec 的所有权问题可以直接从代码判断；动态定义的堆叠身份及远端展示则需要按目标复制模式实测。更易维护的方案是每类异常复用可引用的 GE 定义，用按值 Spec 或引擎句柄管理本次应用，并明确周期与刷新规则。

另一个可直接验算的问题是 `ExecCalc_Damage.cpp:87/216/260` 对闭区间随机整数 1 到 100 使用严格小于。配置 1% 得到 0 个命中值，100% 得到 99 个。统一一个处理边界的概率函数，让策划配置的 0%、1%、100% 与实际区间一致。异常叠层另测同一施法者刷新与不同施法者同时施加。

### ⑥ 持久特效需要能从当前状态重建

`UPassiveNiagaraComponent::OnPassiveActivate` 在收到“开启”且组件已激活时会走 `else`，反而关闭特效。通知来自不可靠多播；组件虽在注册时尝试 `ActivateIfEquipped`，该查询仍依赖本地已有技能规格和初始化标志。短时演出允许偶发缺帧，长期光环则需要后来加入的观察者也能看到正确状态。

**修复与验收**：让“设为开启”重复执行仍保持开启。用观察者可获得的持续 Tag 或持续 GameplayCue 表达光环存在，并在组件就绪时按当前状态初始化。覆盖重复开启、换下和后加入客户端三种情况。类似地，`AbilityActorInfoSet` 的委托订阅与角色重绑也值得成对管理。

## 6. LearnGas 可以怎样借鉴

**先做一项能解释的技能。** 第一轮只需要一个角色、一个目标、Health、一个直接伤害 GE 和一次输入。从 Spec 发出端断点走到属性集，确认服务端扣血一次、客户端最终显示相同值。第二轮再加入鼠标目标、可见投射物、费用和冷却。此时“输入有反应但目标没掉血”已经能拆成授权、激活、瞄准、命中、结算五个可查的问题。

**到换装或重生时引入授予句柄。** 按角色、武器、临时增益区分授予来源，保存每次实际授予的 AbilitySpecHandle 与 ActiveEffectHandle。移除武器时按这一组句柄撤销，保留角色天赋。这里可采用 Lyra 的 Ability Set 思路；当眩晕、沉默、蓄力的阻断关系跨越多个技能时，再引入集中维护的 Tag 关系表。[Epic：Lyra 的 Ability Sets 与 Tag Relationship Mapping](https://dev.epicgames.com/documentation/en-us/unreal-engine/abilities-in-lyra-in-unreal-engine)

**给学习工程加一个“可解释命中”的实验。** 后续开发版可为一次权威命中记录六个量：基础伤害、抗性后、格挡后、护甲后、暴击后、最终生命变化。把同一个命中编号用于关联这份记录与客户端飘字，平时仅在本地调试开启。这样一例“火伤 100 为何只掉 40”就能变成可回归的测试样本。学习收益是把策划公式、网络表现和实际属性变化放到一次可观察的事件里，无需把整份诊断数据复制给每名玩家。

**迁移到 UE 5.8 时逐条核对 API。** 本机 5.8 的 `GameplayAbilitySpec.h` 已将直接访问 `DynamicAbilityTags` 标记为弃用，提供 `GetDynamicSpecSourceTags()`；`Abilities/GameplayAbility.h` 对技能定义标签提供 `GetAssetTags()`。前者是单份已授予规格的可变标签，后者读取技能定义标签。迁移时保留这两个层次，随后再核对 GE 的目标标签组件、激活实例的预测键，以及 Context 的网络往返。

| 进入下一阶段前 | 最小验收场景 |
| --- | --- |
| 基础属性与授予 | 玩家与敌人各接受一次相同伤害；换 Pawn 后核对保留的进度、已撤销的身体技能以及委托触发次数 |
| 可施放的火球 | 双端 PIE 中，拥有者发起施法，服务端确认一次命中；取消施法后按约定检查费用、冷却与技能结束 |
| 技能树与持久化 | 升级、换槽、读档后逐项对照能力标识、等级、槽位与实际被动状态；独立客户端能浏览锁定技能 |
| 范围伤害与联机表现 | 三个距离档复算结果，复制与序列化往返通过；后来加入的观察者能看到已有的长期状态 |

推荐的源码阅读顺序是 `AuraCharacter::InitAbilityActorInfo` → `AbilityInputTagHeld` → `UAuraFireBolt::SpawnProjectiles` → `ApplyDamageEffect` → `UExecCalc_Damage::Execute_Implementation` → `HandleIncomingDamage` → `UOverlayWidgetController::BindCallbacksToDependencies`。第一遍跟数据，第二遍在每个入口旁写下“哪一端运行、对象由谁持有、下次重生后是否仍有效”，就能把这套 ARPG 实现转化成自己的设计判断。

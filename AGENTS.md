# LearnGas 协作约定

本仓库记录 UE 5.8 C++ Gameplay Ability System 的学习过程。每个完成并验证的学习迭代都应在 GitHub 上留下一个可以单独检出的版本。

## 每次迭代

1. 开始前运行 `git status -sb`，确认当前分支、远程跟踪状态和已有的本地修改。保留用户的修改，只暂存本轮涉及的文件。
2. 修改 C++ 或构建配置后编译 `LearnGasEditor Win64 Development`。调整关卡或技能行为后，在编辑器中打开对应关卡并运行 PIE，检查玩家实际看到的效果。
3. 在 `CHANGELOG.md` 顶部追加一条中文记录，写明日期、学习目标、改动、验证结果以及复现所需的关卡或操作。同一天的不同迭代分别记录。
4. 将记录和对应的源码、配置、资产放在同一提交。提交前检查 `git diff --cached --stat`、`git diff --cached --check` 和 `git lfs status`。提交信息写明这次完成的具体内容。
5. 每次完成迭代都推送当前分支。主线使用 `main`，首次推送新分支用 `git push -u origin HEAD`，之后用 `git push`。核对 `git rev-parse HEAD` 与 `git ls-remote origin refs/heads/<当前分支>` 的提交号，最终反馈提交号和 GitHub 链接。推送失败时保留本地提交，说明原因并在恢复后补推。

## 资产与历史

- 关卡和 Unreal 二进制资产使用 Git LFS。提交对应的 `.umap`、`.uasset` 以及相关外部 Actor 或依赖资产。重命名和移动资产后检查引用并一同提交。
- 提交源码、项目描述和共享配置。生成的解决方案、编译产物、缓存、个人配置和凭据留在本机。若以后引入只有二进制的插件，先核实其分发许可及必要文件，再为该插件单独设置跟踪规则。
- 用 `git log --oneline --decorate` 查找版本。需要查看旧版本时，可用 `git worktree add --detach ../LearnGas-review <提交号>` 在另一个目录打开。需要撤销已发布的迭代时，用 `git revert <提交号>` 创建反向提交，记录回退原因并推送，保留共享分支的完整历史。

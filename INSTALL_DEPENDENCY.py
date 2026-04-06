#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
INSTALL_DEPENDENCY.py — 项目依赖自动安装脚本

功能:
    1. 递归拉取所有 Git 子模块（git submodule update --init --recursive）
    2. 根据当前操作系统，执行 SDL_ttf 的外部依赖下载脚本:
       - Windows : Engine/Modules/SDL_ttf/external/Get-GitModules.ps1
       - Linux / macOS : Engine/Modules/SDL_ttf/external/download.sh

使用方法:
    # 在项目根目录下执行
    python INSTALL_DEPENDENCY.py

    # 等价于以下手动步骤:
    #   git submodule update --init --recursive
    #   (Windows)   powershell -ExecutionPolicy Bypass -File Engine/Modules/SDL_ttf/external/Get-GitModules.ps1
    #   (Unix)      bash Engine/Modules/SDL_ttf/external/download.sh

环境要求:
    - Python 3.6+
    - Git 已安装并可在命令行访问
    - (Windows) PowerShell 可用
    - (Unix) bash / sh 可用

注意事项:
    - 请确保在项目根目录（包含 .gitmodules 的目录）下运行此脚本。
    - 脚本会自动将工作目录切换到自身所在的目录，因此也可以从其他位置调用。
"""

import os
import sys
import subprocess
import platform


def get_project_root() -> str:
    """
    获取项目根目录路径。
    以此脚本所在的目录作为项目根目录。
    """
    return os.path.dirname(os.path.abspath(__file__))


def run_command(cmd: list, cwd: str, description: str) -> None:
    """
    运行子进程命令并实时打印输出。

    参数:
        cmd         — 要执行的命令列表, 例如 ["git", "submodule", "update"]
        cwd         — 命令的工作目录
        description — 对这一步操作的描述, 用于日志输出
    """
    print(f"\n{'='*60}")
    print(f"[步骤] {description}")
    print(f"[命令] {' '.join(cmd)}")
    print(f"[目录] {cwd}")
    print(f"{'='*60}")

    result = subprocess.run(cmd, cwd=cwd)

    if result.returncode != 0:
        print(f"\n[错误] 命令执行失败 (退出码: {result.returncode})")
        print(f"[错误] 失败步骤: {description}")
        sys.exit(result.returncode)

    print(f"[完成] {description}")


def init_submodules(project_root: str) -> None:
    """
    步骤 1: 初始化并递归拉取所有 Git 子模块。

    等价于手动执行:
        git submodule update --init --recursive
    """
    run_command(
        cmd=["git", "submodule", "update", "--init", "--recursive"],
        cwd=project_root,
        description="拉取所有 Git 子模块 (git submodule update --init --recursive)",
    )


def download_sdl_ttf_external(project_root: str) -> None:
    """
    步骤 2: 下载 SDL_ttf 的外部依赖（freetype, harfbuzz 等）。

    根据操作系统选择不同的下载脚本:
        - Windows  → 使用 PowerShell 执行 Get-GitModules.ps1
        - Unix系统 → 使用 bash 执行 download.sh
    """
    # SDL_ttf external 目录的路径
    external_dir = os.path.join(project_root, "Engine", "Modules", "SDL_ttf", "external")

    current_os = platform.system()  # "Windows", "Linux", "Darwin"

    if current_os == "Windows":
        # Windows: 通过 PowerShell 执行 Get-GitModules.ps1
        script_path = os.path.join(external_dir, "Get-GitModules.ps1")

        if not os.path.isfile(script_path):
            print(f"[警告] 未找到脚本: {script_path}, 跳过 SDL_ttf 外部依赖下载。")
            return

        run_command(
            cmd=[
                "powershell",
                "-ExecutionPolicy", "Bypass",
                "-File", script_path,
            ],
            cwd=external_dir,
            description="下载 SDL_ttf 外部依赖 (Get-GitModules.ps1)",
        )
    else:
        # Linux / macOS: 通过 bash 执行 download.sh
        script_path = os.path.join(external_dir, "download.sh")

        if not os.path.isfile(script_path):
            print(f"[警告] 未找到脚本: {script_path}, 跳过 SDL_ttf 外部依赖下载。")
            return

        # 确保脚本具有可执行权限
        os.chmod(script_path, 0o755)

        run_command(
            cmd=["bash", script_path],
            cwd=external_dir,
            description="下载 SDL_ttf 外部依赖 (download.sh)",
        )


def main() -> None:
    """
    主函数 — 按顺序执行所有依赖安装步骤。
    """
    project_root = get_project_root()

    print(f"项目根目录: {project_root}")
    print(f"操作系统: {platform.system()} ({platform.platform()})")

    # ---- 步骤 1: 拉取子模块 ----
    init_submodules(project_root)

    # ---- 步骤 2: 下载 SDL_ttf 外部依赖 ----
    download_sdl_ttf_external(project_root)

    # ---- 完成 ----
    print(f"\n{'='*60}")
    print("[完成] 所有依赖安装完毕！")
    print(f"{'='*60}")


if __name__ == "__main__":
    main()

# DkzKext — AMD Radeon RX 7000 (RDNA 3) Support for macOS

<p align="center">
  <strong>Experimental kext para suporte à série AMD Radeon RX 7000 (RDNA 3) no macOS</strong>
</p>

> ⚠️ **EXPERIMENTAL** — Esta kext está em desenvolvimento ativo. Não use em sistemas de produção.

## 📋 Visão Geral

DkzKext é uma kernel extension baseada no framework [Lilu](https://github.com/acidanthera/Lilu) que visa adicionar suporte para GPUs AMD Radeon da série RX 7000 (arquitetura RDNA 3) no macOS. A kext funciona interceptando e modificando os drivers AMD existentes no macOS em tempo de boot.

### GPUs Suportados

| GPU | Chip | Device ID | Spoof → RDNA 2 | Status |
|-----|-------|-----------|----------------|--------|
| RX 7900 XTX | Navi 31 | `0x744C` | Navi 21 (`0x73BF`) | 🔬 Experimental |
| RX 7900 XT | Navi 31 | `0x744C` | Navi 21 (`0x73BF`) | 🔬 Experimental |
| RX 7800 XT | Navi 32 | `0x747E` | Navi 22 (`0x73DF`) | 🔬 Experimental |
| RX 7700 XT | Navi 32 | `0x747E` | Navi 22 (`0x73DF`) | 🔬 Experimental |
| RX 7600 | Navi 33 | `0x7480` | Navi 23 (`0x73FF`) | 🔬 Experimental |
| RX 7600 XT | Navi 33 | `0x7481` | Navi 23 (`0x73FF`) | 🔬 Experimental |

### Compatibilidade macOS

| Versão macOS | Status |
|-------------|--------|
| macOS Ventura (13.x) | 🔬 Experimental |
| macOS Sonoma (14.x) | 🔬 Experimental |
| macOS Sequoia (15.x) | 🔬 Experimental |
| macOS Tahoe (26.x) | 🔬 Experimental |

## 🔧 Como Funciona

DkzKext usa 3 estratégias principais:

### 1. Device ID Spoofing
Modifica o Device ID do GPU RDNA 3 no IORegistry para que os drivers RDNA 2 existentes no macOS reconheçam o hardware. Por exemplo, um RX 7900 XTX (`0x744C`) é apresentado como um RX 6900 XT (`0x73BF`).

### 2. Driver Patching
Intercepta funções dos drivers AMD do macOS em tempo de boot usando o framework Lilu:
- **AMDRadeonX6000.kext** — Driver principal de aceleração
- **AMDRadeonX6000Framebuffer.kext** — Controller de display/framebuffer
- **AMDRadeonX6000HWServices.kext** — Serviços de hardware (HWLibs)
- **AMDSupport.kext** — Framework de suporte AMD

### 3. Property Injection
Injeta propriedades de dispositivo no IORegistry:
- Nome do modelo GPU
- Dados de conectores (DP 2.1, HDMI 2.1)
- Informações de VRAM
- Parâmetros de power management

## 📦 Requisitos para Build

- **macOS** com **Xcode 14+** instalado
- **[Lilu](https://github.com/acidanthera/Lilu)** — Framework de patching (headers necessários)
- **[MacKernelSDK](https://github.com/acidanthera/MacKernelSDK)** — Kernel SDK

### Estrutura de Dependências

```
DkzKext/
├── Lilu.kext              (colocado ao lado ou referenciado via include path)
├── MacKernelSDK/          (clonado como submodule ou copiado)
└── DkzKext/               (este projeto)
```

## 🛠️ Como Compilar

### Método 1: Xcode (Recomendado)

```bash
# 1. Clone as dependências
git clone https://github.com/acidanthera/Lilu.git
git clone https://github.com/acidanthera/MacKernelSDK.git

# 2. Compile o Lilu primeiro
cd Lilu
xcodebuild -configuration Release

# 3. Compile a DkzKext
cd ../DkzKext
xcodebuild -configuration Release \
    LILU_KEXT_PATH=../Lilu/build/Release/Lilu.kext \
    MAC_KERNEL_SDK_PATH=../MacKernelSDK
```

### Método 2: Makefile

```bash
# Configurar paths
export LILU_PATH=../Lilu
export MACKERNELSDK_PATH=../MacKernelSDK

# Compilar
make release
```

## 📥 Instalação (OpenCore)

### 1. Pré-requisitos
- [OpenCore](https://github.com/acidanthera/OpenCorePkg) configurado
- `Lilu.kext` já instalado e funcionando

### 2. Copiar Kexts
```
EFI/OC/Kexts/
├── Lilu.kext              (deve carregar primeiro)
└── DkzKext.kext           (carrega depois do Lilu)
```

### 3. Configurar config.plist
Adicione ao `config.plist` do OpenCore:

```xml
<dict>
    <key>Arch</key>
    <string>x86_64</string>
    <key>BundlePath</key>
    <string>DkzKext.kext</string>
    <key>Comment</key>
    <string>AMD RDNA 3 (RX 7000) Support</string>
    <key>Enabled</key>
    <true/>
    <key>ExecutablePath</key>
    <string>Contents/MacOS/DkzKext</string>
    <key>MaxKernel</key>
    <string></string>
    <key>MinKernel</key>
    <string>22.0.0</string>
    <key>PlistPath</key>
    <string>Contents/Info.plist</string>
</dict>
```

### 4. Device Properties (Opcional)
Adicione ao `DeviceProperties > Add` no config.plist para o path PCI do GPU:

```xml
<key>PciRoot(0x0)/Pci(0x1,0x0)/Pci(0x0,0x0)/Pci(0x0,0x0)/Pci(0x0,0x0)</key>
<dict>
    <key>device-id</key>
    <data>vHMAAA==</data>  <!-- 0x73BF para Navi 31 -> Navi 21 -->
    <key>model</key>
    <string>AMD Radeon RX 7900 XTX</string>
</dict>
```

> **Nota:** Ajuste o path PCI (`PciRoot(...)`) conforme seu hardware. Use o Hackintool para encontrar o path correto.

## ⚙️ Boot Arguments

| Argumento | Descrição |
|-----------|-----------|
| `-dkzoff` | Desativa completamente a DkzKext |
| `-dkzdbg` | Ativa logging detalhado (debug) |
| `-dkzbeta` | Ativa features experimentais/beta |
| `-dkznospoof` | Desativa o Device ID spoofing |
| `-dkzdump` | Faz dump do estado dos registros do GPU |

## 🔍 Troubleshooting

### GPU não é detectado
1. Verifique se `Lilu.kext` está carregando antes da `DkzKext.kext`
2. Confirme o Device ID do seu GPU com `lspci` ou Device Manager
3. Adicione `-dkzdbg` aos boot args e verifique os logs

### Tela preta / sem display
1. Tente adicionar `agdpmod=pikera` nos boot args
2. Verifique se o cabo está conectado na porta correta (tente DP primeiro)
3. Adicione `-dkznospoof` para testar sem spoofing

### Kernel Panic no boot
1. Desative com `-dkzoff` para confirmar que é a DkzKext
2. Verifique a versão do macOS (mínimo: Ventura 13.0)
3. Verifique conflitos com WhateverGreen — use um ou outro, não ambos

### Ver logs do kernel
```bash
# macOS Recovery / Terminal
log show --predicate 'process == "kernel"' --last 5m | grep "DkzKext"

# Ou via serial output
nvram boot-args="-v keepsyms=1 debug=0x100 -dkzdbg"
```

## ⚠️ Limitações Conhecidas

1. **Sem aceleração Metal completa** — Requer drivers Metal proprietários da Apple que não existem para RDNA 3
2. **Power management limitado** — SMU 13.0 (RDNA 3) difere do SMU 11.0/12.0 (RDNA 2)
3. **Possível instabilidade** — O hardware RDNA 3 (GFX11, DCN 3.2) é fundamentalmente diferente do RDNA 2 (GFX10.3, DCN 3.0)
4. **Sem suporte oficial** — Este é um projeto experimental da comunidade

## 📁 Estrutura do Projeto

```
DkzKext/
├── DkzKext/
│   ├── Info.plist              # Bundle config + IOKit matching
│   ├── kern_start.cpp          # Entry point (Lilu plugin)
│   ├── DkzKext.hpp/.cpp        # Classe principal — orquestração
│   ├── NaVi3x.hpp              # Definições de hardware RDNA 3
│   ├── PatcherPlus.hpp         # Utilitários de patching
│   ├── Framebuffer/
│   │   ├── Framebuffer.hpp/.cpp  # Display output & connectors
│   ├── Firmware/
│   │   ├── HWLibs.hpp/.cpp       # Hardware library patches
│   └── PowerPlay/
│       ├── PowerPlay.hpp/.cpp    # Power management
├── Headers/
│   ├── kern_util.hpp           # Macros e helpers
│   └── ATOMBIOS.hpp            # AtomBIOS structures
├── README.md
└── Makefile
```

## 🤝 Contribuindo

Contribuições são bem-vindas! Áreas que precisam de trabalho:

- [ ] Engenharia reversa dos HWLibs para RDNA 3
- [ ] Testes em hardware real (RX 7600, 7700 XT, 7800 XT, 7900 XT/XTX)
- [ ] Patching de DCN 3.2 para display output
- [ ] Suporte a SMU 13.0 para power management
- [ ] Testes em diferentes versões do macOS

## 📄 Licença

Copyright © 2026 DKZ. All rights reserved.

---

**Aviso Legal:** Este software é fornecido "como está", sem garantias de qualquer tipo. O uso é por sua conta e risco. Modificações no kernel podem causar instabilidade do sistema, perda de dados, ou danos ao hardware. Use com responsabilidade.

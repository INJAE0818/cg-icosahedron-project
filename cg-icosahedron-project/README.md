# 🧊 Icosahedron & Truncated Icosahedron Rendering  
### OpenGL 3.3 Core | Computer Graphics Portfolio Project

<p align="center">
  <img src="docs/cg-icosahedron-project.gif" width="430">
</p>

정20면체(Icosahedron)와 깎은 정20면체(Truncated Icosahedron, Soccer Ball)를  
**OpenGL 3.3 Core Profile** 환경에서 기하학적으로 직접 생성하고,  
세 가지 형태를 한 화면에서 비교 렌더링한 프로젝트입니다.

---

## 📌 Features (핵심 구현)

### 1. 정20면체(Icosahedron) 기하 생성
- 황금비 φ = (1 + √5) / 2 기반 12개 정점 생성  
- 20개 삼각형 면(ICO_F) 구성  
- 와이어프레임 & 솔리드 버전 제작  
- 면 단위 색상 팔레트로 구분감 부여  

---

### 2. 깎은 정20면체(축구공) 모델링
정20면체를 1/3 edge split 방식으로 절단하여  
**12개의 정오각형 + 20개의 정육각형**이 있는 축구공 형태로 생성.

- 각 정점의 adjacency list 구성  
- 모서리 1/3 지점을 샘플링해 절단 정점 생성  
- 원본 삼각형 → 육각형 재구성  
- 정점 주변을 오각형 링으로 구성  
- 모든 다각형을 팬 삼각분할로 Triangulation  

---

### 3. Z-Fighting 없는 안정적 레이어 렌더링
- 오각형 → 육각형 → 윤곽선 순서 렌더링  
- `glPolygonOffset()`로 깊이 충돌 방지  

---

### 4. 다중 모델 배치 및 독립 변환 적용
- 상단 중앙: 평면 + 와이어프레임 정20면체  
- 좌하단: 솔리드 정20면체  
- 우하단: 축구공 모델  
- 각 모델은 개별 Transform(회전/스케일) 적용  

---

### 5. 사용자 인터랙션
- **Space** : 자동 회전 On/Off  
- **Left Mouse** : 회전 축 변경 (X → Y → Z)  
- **I** : 회전값 초기화  
- **Z / X** : 축소 / 확대  

윈도우 타이틀에 Spin/Axis/Scale 정보가 실시간 반영됩니다.

---

## 🏗 Project Structure


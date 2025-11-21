# CL-WeMeet  
**WeMeet 스타일 실시간 협업 웹 플랫폼**

![메인 화면](media/Demo_Video.gif)

> “실시간 채팅 · 인터랙티브 퀴즈 · 웹툰 추천 · 팀 소개까지 한 번에!”  



## 프로젝트 한눈에 보기

| 메인 화면 & UI                                     |
|--------------------------------------------------|
| ![Main](images/최종.png)                          |
|--------------------------------------------------|
| 우리팀 순간들                                    |
|--------------------------------------------------|
![Team1](assets/Pt_moment_01.JPG) ![Team2](assets/Pt_moment_02.JPG) ![Team3](assets/Pt_moment_03.JPG) |

## 주요 기능

| 기능               | 설명                                              | 미리보기                                      |
|--------------------|---------------------------------------------------|-----------------------------------------------|
| 실시간 채팅        | Node.js + Express 기반 완전 동작 채팅 시스템      | ![채팅](img/chatbot.png)                     |
| 인터랙티브 퀴즈    | JSON 데이터로 랜덤 출제 → 즉시 정답 확인 및 점수   | ![퀴즈](img/quizboard.png)                   |
| 웹툰 추천 슬라이드  | 인기 웹툰 자동 슬라이드 + 클릭 시 상세 보기       | ![웹툰](img/webtoon.png)                     |


## 기술 스택

```text
Frontend    → HTML5, CSS3, Vanilla JavaScript
Backend     → Node.js, Express.js
Server      → server.js (포트 3000)
Data        → JSON (questions.json, quiz.json)
Styling     → WeMeet을 모티브로 한 고급 UI/UX

프로젝트 구조
cl-wemeet-project/
├── assets/           팀 순간 사진 (Pt_moment_01~03.JPG)
├── css/              style.css
├── data/             questions.json (퀴즈 데이터)
├── images/           배경, 버튼 이미지
├── img/              아이콘, 팀원 사진, 심볼
├── js/               main.js, quiz.js
├── media/            Demo_Video.gif
├── node_modules/     Express 등
├── server.js         실시간 채팅 서버
├── package.json
└── *.html            home, chatbot, quiz, webtoon, team 등

빠른 실행 방법
# 1. 클론
git clone https://github.com/INJAE0818/Choi_Projects.git
cd Choi_Projects/cl-wemeet-project

# 2. 의존성 설치
npm install

# 3. 서버 실행 (채팅 기능 사용하려면 필수!)
node server.js

# 4. 접속
http://localhost:3000
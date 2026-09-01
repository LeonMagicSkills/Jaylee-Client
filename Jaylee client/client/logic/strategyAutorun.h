#pragma once

// Запустить автозапуск стратегии, беря номер стратегии из config.cfg (ключ Strategy=)
void StartStrategyAutorunFromConfig();
// Остановить поведение автозапуска (если было запущено)
void StopStrategyAutorun();

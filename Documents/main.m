M = xlsread('C:\Users\binhc\OneDrive\Desktop\ACD Workplace\Documents\Weight data.xlsx',1,'D7:B13');
RE   = xlsread('C:\Users\binhc\OneDrive\Desktop\ACD Workplace\Documents\Weight data.xlsx',1,'E7:E13');
SD   = xlsread('C:\Users\binhc\OneDrive\Desktop\ACD Workplace\Documents\Weight data.xlsx',1,'I7:I13');

figure(1)
set(0, 'DefaultFigurePosition', [0 0 600 300]);
plot(M, RE,'LineWidth',1.5);
yline(0.1, 'LineWidth', 1, 'LineStyle', '--', 'Color', 'r');
yline(0, 'LineWidth', 1, 'LineStyle', '-', 'Color', 'b');
xlabel("Weight (g)");
ylabel("Relative error (%)");

axis([0 750 -1 2])

%avg = mean(SD,1)
figure(2)
set(0, 'DefaultFigurePosition', [0 0 600 300]);
plot(M, SD,'LineWidth',1.5);
yline(max(SD), 'LineWidth', 1, 'LineStyle', '--', 'Color', 'b');
yline(min(SD), 'LineWidth', 1, 'LineStyle', '--', 'Color', 'b');
xlabel("Weight (g)");
ylabel("Standard deviation (g)");

axis([0 750 0 0.15])